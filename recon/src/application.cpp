/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <chrono>
#include <exception>

#include <spdlog/spdlog.h>

#include "common/scoped_timer.hpp"
#include "common/utils.hpp"
#include "recon/application.hpp"
#include "recon/encoder.hpp"
#include "recon/monitor.hpp"
#include "recon/preprocessing.hpp"
#include "recon/preprocessor.hpp"
#include "recon/projection_mediator.hpp"
#include "recon/rpc_server.hpp"
#include "recon/slice_mediator.hpp"

namespace recastx::recon {

using namespace std::chrono_literals;
using namespace std::string_literals;

Application::Application(size_t raw_buffer_size, 
                         const ImageprocParams& imageproc_params, 
                         DaqClientInterface* daq_client,
                         FilterFactory* ramp_filter_factory,
                         ReconstructorFactory* recon_factory,
                         const RpcServerConfig& rpc_config)
     : raw_buffer_(raw_buffer_size),
       monitor_(new Monitor()),
       proj_mediator_(new ProjectionMediator(k_PROJECTION_MEDIATOR_BUFFER_SIZE)),
       slice_mediator_(new SliceMediator()),
       imgproc_params_(imageproc_params),
       preproc_(new Preprocessor(ramp_filter_factory, imageproc_params.num_threads)),
       recon_factory_(recon_factory),
       scan_mode_(rpc::ScanMode_Mode_DISCRETE),
       scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL),
       daq_client_(daq_client),
       rpc_server_(new RpcServer(rpc_config.port, this)) {
}

Application::~Application() { 
    closing_ = true;
    for (auto& t : consumer_threads_) t.join();
}

void Application::setProjectionGeometry(BeamShape beam_shape, size_t col_count, size_t row_count,
                                        float pixel_width, float pixel_height,
                                        float src2origin, float origin2det, size_t num_angles) {
    proj_geom_ = {beam_shape, col_count, row_count, pixel_width, pixel_height, 
                  src2origin, origin2det, defaultAngles(num_angles)};
}

void Application::setReconGeometry(std::optional<size_t> slice_size, std::optional<size_t> volume_size,
                                   std::optional<float> minx, std::optional<float> maxx, 
                                   std::optional<float> miny, std::optional<float> maxy, 
                                   std::optional<float> minz, std::optional<float> maxz) {
    slice_size_ = slice_size;
    volume_size_ = volume_size;
    min_x_ = minx;
    max_x_ = maxx;
    min_y_ = miny;
    max_y_ = maxy;
    min_z_ = minz;
    max_z_ = maxz;
    // initialization is delayed
}

bool Application::tryComputeReciprocal() {
    if (darks_.empty() && flats_.empty()) {
        raw_buffer_.reset();
        spdlog::warn("Send dark and flat images first! Received projections ignored.");
        return false;
    }

    spdlog::info("Computing reciprocal for flat field correction "
                 "with {} darks and {} flats ...", darks_.size(), flats_.size());

    {
#if (VERBOSITY >= 2)
        ScopedTimer timer("Bench", "Computing reciprocal");
#endif

        auto [dark_avg, reciprocal] = recastx::recon::computeReciprocal(darks_, flats_);

        spdlog::debug("Downsampling dark ... ");
        downsample(dark_avg, dark_avg_);
        spdlog::debug("Downsampling reciprocal ... ");
        downsample(reciprocal, reciprocal_);
    }

    arrayStat(reciprocal_, reciprocal_.shape(), "Reciprocal");

    reciprocal_computed_ = true;
    spdlog::info("Reciprocal computed!");

    return true;
}

void Application::setPipelinePolicy(bool wait_on_slowness) {
    pipeline_wait_on_slowness_ = wait_on_slowness;
}

void Application::startConsuming() {
    for (size_t i = 0; i < 2 * daq_client_->concurrency(); ++i) {
        consumer_threads_.emplace_back(&Application::consume, this);
    }
}

void Application::startPreprocessing() {
    auto t = std::thread([&] {
        while (!closing_) {
            if (waitForProcessing()) continue;

            if (!raw_buffer_.fetch(100)) continue;

            {
                std::lock_guard lck(reciprocal_mtx_);
                if (!reciprocal_computed_) {
                    if (!tryComputeReciprocal()) continue;
                }
            }

            spdlog::info("Preprocessing - started");

            preproc_->process(raw_buffer_, sino_buffer_, dark_avg_, reciprocal_);

            while (!sino_buffer_.tryPrepare(100)) {
                if (closing_) return;
            }

            spdlog::info("Preprocessing - finished");
        }
    });

    t.detach();
}

void Application::startUploading() {
    auto t = std::thread([&] {
        while (!closing_) {
            if(waitForProcessing()) continue;

            if (!sino_buffer_.fetch(100)) continue;

            spdlog::info("Uploading sinograms to GPU - started");
            size_t chunk_size = sino_buffer_.front().shape()[0];
            {
                if (scan_mode_ == rpc::ScanMode_Mode_DISCRETE) {
                    recon_->uploadSinograms(1 - gpu_buffer_index_, sino_buffer_.front().data(), chunk_size);

                    std::lock_guard<std::mutex> lck(gpu_mtx_);
                    gpu_buffer_index_ = 1 - gpu_buffer_index_;
                    sino_uploaded_ = true;
                } else {
                    std::lock_guard<std::mutex> lck(gpu_mtx_);
                    recon_->uploadSinograms(gpu_buffer_index_, sino_buffer_.front().data(), chunk_size);
                    sino_uploaded_ = true;
                }

                sino_initialized_ = true;
                spdlog::info("Uploading sinograms to GPU - finished");
            }

            gpu_cv_.notify_one();
        }
    });

    t.detach();
}

void Application::startReconstructing() {

    auto t = std::thread([&] {
        while (!closing_) {
            {
                std::unique_lock<std::mutex> lck(gpu_mtx_);
                if (gpu_cv_.wait_for(lck, 10ms, [&] { return sino_uploaded_; })) {
                    if (volume_required_) {
                        spdlog::info("Reconstruction (volume and slices) - started");
                        recon_->reconstructVolume(gpu_buffer_index_, volume_buffer_.back());
                    } else {
                        spdlog::info("Reconstruction (slices) - started");
                    }

                } else {
                    if (waitForSinoInitialization()) continue;
                    slice_mediator_->reconOnDemand(recon_.get(), gpu_buffer_index_);
                    continue;
                }

                slice_mediator_->reconAll(recon_.get(), gpu_buffer_index_);

                sino_uploaded_ = false;
            }
            
            spdlog::info("Reconstruction - finished");
            monitor_->countTomogram();

            if (volume_buffer_.prepare()) {
                spdlog::debug("Reconstructed volume dropped due to slowness of clients");
            }
        }

    });

    t.detach();
}

void Application::consume() {
    Projection<> proj;
    while (!closing_) {
        if (!daq_client_->next(proj)) continue;

        switch(proj.type) {
            case ProjectionType::PROJECTION: {
                if (server_state_ == rpc::ServerState_State_PROCESSING) {
                    pushProjection(proj);
                }

                proj_mediator_->push(std::move(proj));
                monitor_->countProjection();
                break;
            }
            case ProjectionType::DARK: {
                std::lock_guard lck(reciprocal_mtx_);
                if (server_state_ == rpc::ServerState_State_PROCESSING) {
                    maybeResetDarkAndFlatAcquisition();
                    pushDark(std::move(proj));
                }
                monitor_->countDark();
                break;
            }
            case ProjectionType::FLAT: {
                std::lock_guard lck(reciprocal_mtx_);
                if (server_state_ == rpc::ServerState_State_PROCESSING) {
                    maybeResetDarkAndFlatAcquisition();
                    pushFlat(std::move(proj));
                }
                monitor_->countFlat();
                break;
            }
            default:
                throw std::runtime_error("Unexpected projection type");
        }
    }
}

void Application::spin(rpc::ServerState_State state) {
    startReconstructing();
    startUploading();
    startPreprocessing();
    startConsuming();

    daq_client_->start();

    if (state == rpc::ServerState_State_ACQUIRING) {
        startAcquiring();
    } else if (state == rpc::ServerState_State_PROCESSING) {
        startProcessing();
    } else if (state == rpc::ServerState_State_READY) {
        server_state_ = rpc::ServerState_State_READY;
    } else {
        throw std::runtime_error(fmt::format(
                "Cannot start reconstruction server from state: {}", server_state_));
    }

    rpc_server_->start();

    while (!closing_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void Application::setDownsampling(uint32_t col, uint32_t row) {
    imgproc_params_.downsampling_col = col;
    imgproc_params_.downsampling_row = row;

    spdlog::debug("Set projection downsampling: {} / {}", col, row);
}

void Application::setRampFilter(std::string filter_name) {
    imgproc_params_.ramp_filter.name = std::move(filter_name);

    spdlog::debug("Set ramp filter: {}", filter_name);
}

void Application::setProjectionReq(size_t id) {
    proj_mediator_->setId(id); 
}

void Application::setSliceReq(size_t timestamp, const Orientation& orientation) {
    slice_mediator_->update(timestamp, orientation);
}

void Application::setVolumeReq(bool required) {
    volume_required_ = required;
}

void Application::setScanMode(rpc::ScanMode_Mode mode, uint32_t update_interval) {
    scan_mode_ = mode;
    scan_update_interval_ = update_interval;
    
    if (mode == rpc::ScanMode_Mode_DISCRETE) {
        spdlog::debug("Set scan mode: {}", static_cast<int>(mode));
    } else {
        spdlog::debug("Set scan mode: {}, update interval {}", 
                      static_cast<int>(mode), update_interval);
    }
}

void Application::startAcquiring() {
    if (server_state_ == rpc::ServerState_State_ACQUIRING) {
        spdlog::warn("Server already in state ACQUIRING");
        return;
    }
    if (server_state_ == rpc::ServerState_State_PROCESSING) {
        spdlog::warn("Server already in state PROCESSING");
        return;
    }

    initParams();

    daq_client_->startAcquiring();

    server_state_ = rpc::ServerState_State_ACQUIRING;
    spdlog::info("Start acquiring data");

    monitor_.reset(new Monitor(proj_geom_.row_count * proj_geom_.col_count * sizeof(RawDtype), group_size_));
}

void Application::stopAcquiring() {
    if (server_state_ != rpc::ServerState_State_ACQUIRING) {
        spdlog::warn("Server not in state ACQUIRING");
        return;
    }

    daq_client_->stopAcquiring();
    server_state_ = rpc::ServerState_State_READY;
    spdlog::info("Stop acquiring data");

    proj_mediator_->reset();
    monitor_->summarize();
}

void Application::startProcessing() {
    if (server_state_ == rpc::ServerState_State_PROCESSING) {
        spdlog::warn("Server already in state PROCESSING");
        return;
    }

    init();

    daq_client_->startAcquiring();
    server_state_ = rpc::ServerState_State_PROCESSING;
    spdlog::info("Start acquiring and processing data:");

    if (scan_mode_ == rpc::ScanMode_Mode_CONTINUOUS) {
        spdlog::info("- Scan mode: continuous");
        spdlog::info("- Update interval: {}", scan_update_interval_);
    } else if (scan_mode_ == rpc::ScanMode_Mode_DISCRETE) {
        spdlog::info("- Scan mode: discrete");
    }

    monitor_.reset(new Monitor(proj_geom_.row_count * proj_geom_.col_count * sizeof(RawDtype), group_size_));
}

void Application::stopProcessing() {
    if (server_state_ != rpc::ServerState_State_PROCESSING) {
        spdlog::warn("Server not in state PROCESSING");
        return;
    }

    daq_client_->stopAcquiring();
    server_state_ = rpc::ServerState_State_READY;
    spdlog::info("Stop acquiring and processing data");

    proj_mediator_->reset();
    monitor_->summarize();
}

std::optional<rpc::ProjectionData> Application::getProjectionData(int timeout) {
    ProjectionMediator::DataType proj;
    if (proj_mediator_->waitAndPop(proj, timeout)) {
        auto [y, x] = proj.data.shape();
        return createProjectionDataPacket(proj.index % proj_geom_.angles.size(), x, y, proj.data);
    }
    return std::nullopt;
}

std::vector<rpc::ReconData> Application::getVolumeData(int timeout) {
    if (volume_buffer_.fetch(timeout)) {
        auto& data = volume_buffer_.front();
        auto [x, y, z] = data.shape();
        return createVolumeDataPacket(data, x, y, z);
    }
    return {};
}

std::vector<rpc::ReconData> Application::getSliceData(int timeout) {
    std::vector<rpc::ReconData> ret;
    auto& buffer = slice_mediator_->allSlices();
    if (buffer.fetch(timeout)) {
        for (auto& [k, slice] : buffer.front()) {
            auto& data = std::get<2>(slice);
            auto [x, y] = data.shape();
            ret.emplace_back(createSliceDataPacket(data, x, y, std::get<1>(slice)));
        }
    }
    return ret;
}

std::vector<rpc::ReconData> Application::getOnDemandSliceData(int timeout) {
    std::vector<rpc::ReconData> ret;
    auto& buffer = slice_mediator_->onDemandSlices();
    if (buffer.fetch(timeout)) {
        for (auto& [k, slice] : buffer.front()) {
            if (std::get<0>(slice)) {
                auto& data = std::get<2>(slice);
                auto& shape = data.shape();
                ret.emplace_back(createSliceDataPacket(data, shape[0], shape[1], std::get<1>(slice)));
            }
        }
    }
    return ret;
}

void Application::init() {
    spdlog::info("Initial parameters for real-time 3D tomographic reconstruction:");

    initParams();

    uint32_t downsampling_col = imgproc_params_.downsampling_col;
    uint32_t downsampling_row = imgproc_params_.downsampling_row;
    size_t col_count = proj_geom_.col_count / downsampling_col;
    size_t row_count = proj_geom_.row_count / downsampling_row;

    maybeInitFlatFieldBuffer(row_count, col_count);

    preproc_->init(raw_buffer_, col_count, row_count, imgproc_params_, paganin_cfg_);

    maybeInitReconBuffer(col_count, row_count);

    initReconstructor(col_count, row_count);

    spdlog::info("- Number of projection images per tomogram: {}", proj_geom_.angles.size());
    spdlog::info("- Projection image size: {} ({}) x {} ({})", 
                 col_count, downsampling_col, row_count, downsampling_row);
}

void Application::initParams() {
    group_size_ = (scan_mode_ == rpc::ScanMode_Mode_CONTINUOUS 
                   ? scan_update_interval_ : proj_geom_.angles.size());
    proj_mediator_->setFilter(group_size_);

    sino_initialized_ = false;
    gpu_buffer_index_ = 0;
}

void Application::initReconstructor(size_t col_count, size_t row_count) {
    auto [min_x, max_x] = details::parseReconstructedVolumeBoundary(min_x_, max_x_, col_count);
    auto [min_y, max_y] = details::parseReconstructedVolumeBoundary(min_y_, max_y_, col_count);
    auto [min_z, max_z] = details::parseReconstructedVolumeBoundary(min_z_, max_z_, row_count);
    
    size_t s_size = slice_size_.value_or(expandDataSizeForGpu(col_count, 64));
    size_t p_size = volume_size_.value_or(128);
    float half_slice_height = 0.5f * (max_z - min_z) / p_size;
    float z0 = 0.5f * (max_z + min_z);

    slice_geom_ = {s_size, s_size, 1, min_x, max_x, min_y, max_y, z0 - half_slice_height, z0 + half_slice_height};
    volume_geom_ = {p_size, p_size, p_size, min_x, max_x, min_y, max_y, min_y, max_y};

    volume_buffer_.resize({volume_geom_.col_count, volume_geom_.row_count, volume_geom_.slice_count});
    slice_mediator_->resize({slice_geom_.col_count, slice_geom_.row_count});

    bool double_buffering = scan_mode_ == rpc::ScanMode_Mode_DISCRETE;
    recon_ = recon_factory_->create(
        col_count, row_count, proj_geom_, slice_geom_, volume_geom_, double_buffering);
}

void Application::maybeInitFlatFieldBuffer(size_t row_count, size_t col_count) {
    size_t row_count_orig = proj_geom_.row_count;
    size_t col_count_orig = proj_geom_.col_count;

    if (!darks_.empty()) {
        auto& shape = darks_[0].shape();
        if (shape[0] != row_count_orig || shape[1] != col_count_orig) {
            darks_.clear();
            spdlog::debug("Dark image buffer reset");
        }
    }

    if (!flats_.empty()) {
        auto& shape = flats_[0].shape();
        if (shape[0] != row_count_orig || shape[1] != col_count_orig) {
            flats_.clear();
            spdlog::debug("Flat image buffer reset");
        }
    }

    auto shape = dark_avg_.shape();
    if (shape[0] != row_count || shape[1] != col_count) {
        dark_avg_.resize({row_count, col_count});
        reciprocal_.resize({row_count, col_count});
        spdlog::debug("Reciprocal buffer resized");
    }

    reciprocal_computed_ = false;
}

void Application::maybeInitReconBuffer(size_t col_count, size_t row_count) {
    auto shape = raw_buffer_.shape();
    if (shape[0] != group_size_ || shape[1] != row_count || shape[2] != col_count) {
        raw_buffer_.resize({group_size_, row_count, col_count});
        sino_buffer_.resize({group_size_, row_count, col_count});
        spdlog::debug("Reconstruction buffers resized");
    }
    raw_buffer_.reset();
}

void Application::maybeResetDarkAndFlatAcquisition() {
    if (reciprocal_computed_) {
        raw_buffer_.reset();
        darks_.clear();
        flats_.clear();
        reciprocal_computed_ = false;
        spdlog::info("Re-collecting dark and flat images");
    }
}

void Application::pushProjection(const Projection<>& proj) {
    if (pipeline_wait_on_slowness_ && raw_buffer_.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (raw_buffer_.occupied() == 0) {
        monitor_->resetPerf();
    }

    raw_buffer_.fill<RawDtype>(
        proj.index, reinterpret_cast<const char*>(proj.data.data()), proj.data.shape());
}


} // namespace recastx::recon
