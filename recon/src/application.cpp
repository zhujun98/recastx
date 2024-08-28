/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <chrono>
#include <exception>

#if defined(BENCHMARK)
#include "nvtx3/nvtx3.hpp"
#endif

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
#include "recon/cuda/sinogram_proxy.cuh"
#include "recon/cuda/volume_proxy.cuh"

namespace recastx::recon {

#define PROJECTION_EXPANSION 32
#define SLICE_EXPANSION 32

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
       slice_mediator_(new SliceMediator),
       sino_proxy_(new SinogramProxy),
       volume_proxy_(new VolumeProxy),
       imgproc_params_(imageproc_params),
       preproc_(new Preprocessor(ramp_filter_factory, imageproc_params.num_threads)),
       recon_factory_(recon_factory),
       scan_mode_(rpc::ScanMode_Mode_STATIC),
       scan_update_interval_(K_MAX_SCAN_UPDATE_INTERVAL),
       daq_client_(daq_client),
       rpc_server_(new RpcServer(rpc_config.port, this)) {
}

Application::~Application() { 
    closing_ = true;
    for (auto& t : consumer_threads_) t.join();
}

void Application::setProjectionGeometry(BeamShape beam_shape, uint32_t col_count, uint32_t row_count,
                                        float pixel_width, float pixel_height,
                                        float src2origin, float origin2det,
                                        uint32_t angle_count, AngleRange angle_range) {
    beam_shape_ = beam_shape;
    orig_col_count_ = col_count == 0 ? orig_col_count_ : col_count;
    orig_row_count_ = row_count == 0 ? orig_row_count_ : row_count;
    pixel_width_ = pixel_width;
    pixel_height_ = pixel_height;
    source2origin_ = src2origin;
    origin2detector_ = origin2det;
    angle_count_ = angle_count == 0 ? angle_count_ : angle_count;
    angle_range_ = angle_range;
}

void Application::setReconGeometry(std::optional<uint32_t> slice_size, std::optional<uint32_t> volume_size,
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

        recastx::recon::computeReciprocal(darks_, flats_, dark_avg_, reciprocal_,
                                          {imgproc_params_.downsampling_row, imgproc_params_.downsampling_col});
    }

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

            spdlog::debug("Preprocessing - started");

            {
#if defined(BENCHMARK)
                nvtx3::scoped_range sr("Preprocessing projections");
#endif
                preproc_->process(raw_buffer_, sino_proxy_->buffer(), dark_avg_, reciprocal_, imgproc_params_.offset);
            }


#if defined(BENCHMARK)
            nvtx3::scoped_range sr("Waiting for sinogram buffer ready");
#endif
            while (!sino_proxy_->tryPrepareBuffer(100)) {
                if (closing_) return;
            }

            spdlog::debug("Preprocessing - finished");
        }
    });

    t.detach();
}

void Application::startUploading() {

    auto t = std::thread([&] {
        while (!closing_) {
            if(waitForProcessing()) continue;

            if (!sino_proxy_->fetchData(100)) continue;

            {
                spdlog::debug("Uploading sinograms to GPU - started");

#if defined(BENCHMARK)
                nvtx3::scoped_range sr("Uploading sinograms to GPU");
#endif

                if (double_buffering_) {
                    recon_->uploadSinograms(1 - gpu_buffer_index_, sino_proxy_.get());

                    std::lock_guard<std::mutex> lck(recon_mtx_);
                    gpu_buffer_index_ = 1 - gpu_buffer_index_;
                    sino_uploaded_ = true;
                } else {
                    std::lock_guard<std::mutex> lck(recon_mtx_);
                    recon_->uploadSinograms(gpu_buffer_index_, sino_proxy_.get());
                    sino_uploaded_ = true;
                }

                sino_initialized_ = true;

                spdlog::debug("Uploading sinograms to GPU - finished");
            }

            recon_cv_.notify_one();
        }
    });

    t.detach();
}


void Application::startReconstructing() {

    auto t = std::thread([&] {
        while (!closing_) {
            {
                std::unique_lock<std::mutex> lck(recon_mtx_);
                if (recon_cv_.wait_for(lck, 10ms, [&] { return sino_uploaded_; })) {

                    if (volume_required_) {
                        spdlog::debug("Reconstructing volume - started");

#if defined(BENCHMARK)
                        nvtx3::scoped_range sr("Reconstructing volume");
#endif

                        recon_->reconstructVolume(gpu_buffer_index_, volume_proxy_->buffer());
                    }

                    spdlog::debug("Reconstructing slices - started");


#if defined(BENCHMARK)
                    nvtx3::scoped_range sr("Reconstructing all slices");
#endif

                    slice_mediator_->reconAll(recon_.get(), gpu_buffer_index_);

                    sino_uploaded_ = false;

                    monitor_->countTomogram();

                    spdlog::debug("Reconstructing - finished");

                    if (volume_proxy_->prepareBuffer()) {
                        spdlog::debug("Reconstructed volume dropped due to slowness of clients");
                    }

                } else {
                    if (sino_initialized_) {
#if defined(BENCHMARK)
                        nvtx3::scoped_range sr("Reconstructing on-demand slices");
#endif
                        slice_mediator_->reconOnDemand(recon_.get(), gpu_buffer_index_);
                    }

                    continue;
                }
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
                if (monitor_->numProjections() == 0) monitor_->resetPerf();
                monitor_->countProjection();
                break;
            }
            case ProjectionType::DARK: {
                std::lock_guard lck(reciprocal_mtx_);
                if (server_state_ == rpc::ServerState_State_PROCESSING) {
                    maybeResetDarkAndFlatAcquisition();
                    pushDark(std::move(proj));
                }
                if (monitor_->numProjections() != 0) monitor_->reset();
                monitor_->countDark();
                break;
            }
            case ProjectionType::FLAT: {
                std::lock_guard lck(reciprocal_mtx_);
                if (server_state_ == rpc::ServerState_State_PROCESSING) {
                    maybeResetDarkAndFlatAcquisition();
                    pushFlat(std::move(proj));
                }
                if (monitor_->numProjections() != 0) monitor_->reset();
                monitor_->countFlat();
                break;
            }
            default:
                throw std::runtime_error("Unexpected projection type");
        }
    }
}

void Application::spin() {
    startReconstructing();
    startUploading();
    startPreprocessing();
    startConsuming();

    if (server_state_ == rpc::ServerState_State_UNKNOWN) {
        server_state_ = rpc::ServerState_State_READY;
    }

    daq_client_->spin();
    rpc_server_->spin();
}

void Application::setDownsampling(uint32_t col, uint32_t row) {
    imgproc_params_.downsampling_col = col;
    imgproc_params_.downsampling_row = row;

    spdlog::debug("Set projection downsampling: {} / {}", col, row);
}

void Application::setCorrection(int offset, bool minus_log) {
    imgproc_params_.offset = offset;
    imgproc_params_.minus_log = minus_log;

    spdlog::debug("Set projection center correction: {}", offset);
    spdlog::debug("Set minus log: {}", minus_log ? "enabled" : "disabled");
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

    if (mode != rpc::ScanMode_Mode_CONTINUOUS) {
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

    server_state_ = rpc::ServerState_State_ACQUIRING;
    spdlog::info("Preparing for acquiring data");

    monitor_.reset(new Monitor(0, group_size_));

    daq_client_->setAcquiring(true);
}

void Application::stopAcquiring() {
    if (server_state_ != rpc::ServerState_State_ACQUIRING) {
        spdlog::warn("Server not in state ACQUIRING");
        return;
    }

    daq_client_->setAcquiring(false);

    server_state_ = rpc::ServerState_State_READY;
    spdlog::info("Stopping acquiring data");

    proj_mediator_->reset();
    monitor_->summarize();
}

void Application::startProcessing() {
    if (server_state_ == rpc::ServerState_State_PROCESSING) {
        spdlog::warn("Server already in state PROCESSING");
        return;
    }

    init();

    server_state_ = rpc::ServerState_State_PROCESSING;
    spdlog::info("Preparing for acquiring and processing data:");

    if (scan_mode_ == rpc::ScanMode_Mode_CONTINUOUS) {
        spdlog::info("- Scan mode: continuous");
        spdlog::info("- Update interval: {}", scan_update_interval_);
    } else if (scan_mode_ == rpc::ScanMode_Mode_DYNAMIC) {
        spdlog::info("- Scan mode: dynamic");
    } else if (scan_mode_ == rpc::ScanMode_Mode_STATIC) {
        spdlog::info("- Scan mode: static");
    }

    monitor_.reset(new Monitor(orig_col_count_ * orig_row_count_ * sizeof(RawDtype) * group_size_, group_size_));

    daq_client_->startAcquiring(orig_row_count_, orig_col_count_);
}

void Application::stopProcessing() {
    if (server_state_ != rpc::ServerState_State_PROCESSING) {
        spdlog::warn("Server not in state PROCESSING");
        return;
    }

    daq_client_->setAcquiring(false);

    // TODO: find a better solution to empty the DAQ buffer
    std::this_thread::sleep_for(std::chrono::microseconds(1000));

    server_state_ = rpc::ServerState_State_READY;
    spdlog::info("Stopping acquiring and processing data");

    proj_mediator_->reset();
    monitor_->summarize();
}

std::optional<rpc::ProjectionData> Application::getProjectionData(int timeout) {
    ProjectionMediator::DataType proj;
    if (proj_mediator_->waitAndPop(proj, timeout)) {
        auto [y, x] = proj.data.shape();
        auto mod = angle_count_ == 0 ? 1 : angle_count_;
        return createProjectionDataPacket(proj.index % mod, x, y, proj.data);
    }
    return std::nullopt;
}

std::vector<rpc::ReconData> Application::getVolumeData(int timeout) {
    auto data = volume_proxy_->fetchData(timeout);
    if (data.ptr != nullptr) {
        return createVolumeDataPacket(data.ptr, data.x, data.y, data.z);
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
    spdlog::info("[Init] ------------------------------------------------------------");

    checkParams();

    initParams();

    uint32_t ds_col = imgproc_params_.downsampling_col;
    uint32_t ds_row = imgproc_params_.downsampling_row;

#if defined(PROJECTION_EXPANSION)
    size_t col_count = expandDataSize(orig_col_count_ / ds_col, PROJECTION_EXPANSION);
    size_t row_count = expandDataSize(orig_row_count_ / ds_row, PROJECTION_EXPANSION);
#else
    size_t col_count = orig_col_count_ / ds_col;
    size_t row_count = orig_row_count_ / ds_row;
#endif

    spdlog::info("[Init] - Projection size: {} ({}/{}) x {} ({}/{})",
                 col_count, orig_col_count_, ds_col, row_count, orig_row_count_, ds_row);
    spdlog::info("[Init] - Number of projections per scan: {}", angle_count_);

    maybeInitFlatFieldBuffer(row_count, col_count);

    maybeInitDataBuffer(col_count, row_count);

    initReconstructor(col_count, row_count);

    preproc_->init(raw_buffer_, col_count, row_count, imgproc_params_, paganin_cfg_);

    spdlog::info("[Init] ------------------------------------------------------------");
}

void Application::checkParams() {
    if (orig_col_count_ <= 0 || orig_row_count_ <= 0 || angle_count_ <= 0) {
        throw std::invalid_argument("Col count / Row count / Angle count must all be positive!");
    }
}

void Application::initParams() {
    group_size_ = (scan_mode_ == rpc::ScanMode_Mode_CONTINUOUS || angle_count_ == 0)
            ? scan_update_interval_ : angle_count_;
    proj_mediator_->setFilter(group_size_);

    sino_initialized_ = false;
    gpu_buffer_index_ = 0;
}

void Application::maybeInitFlatFieldBuffer(uint32_t row_count, uint32_t col_count) {
    if (!darks_.empty()) {
        auto& shape = darks_[0].shape();
        if (shape[0] != orig_row_count_ || shape[1] != orig_col_count_) {
            darks_.clear();
            spdlog::debug("Dark image buffer reset");
        }
    }

    if (!flats_.empty()) {
        auto& shape = flats_[0].shape();
        if (shape[0] != orig_row_count_ || shape[1] != orig_col_count_) {
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

void Application::maybeInitDataBuffer(uint32_t col_count, uint32_t row_count) {
    double sino_size = col_count * row_count * angle_count_ * sizeof(ProDtype) / static_cast<double>(1024 * 1024);
    spdlog::info("[Init] - Sinogram size: {:.1f} MB", sino_size);

    auto shape = raw_buffer_.shape();
    if (shape[0] != group_size_ || shape[1] != row_count || shape[2] != col_count) {
        raw_buffer_.resize({group_size_, row_count, col_count});
        sino_proxy_->reshapeBuffer({group_size_, row_count, col_count});
        spdlog::debug("Reconstruction buffers resized");
    }
    sino_proxy_->setAngleCount(angle_count_);
    raw_buffer_.reset();
    sino_proxy_->reset();
}

void Application::initReconstructor(uint32_t col_count, uint32_t row_count) {
    auto [min_x, max_x] = details::parseReconstructedVolumeBoundary(min_x_, max_x_, col_count);
    auto [min_y, max_y] = details::parseReconstructedVolumeBoundary(min_y_, max_y_, col_count);
    auto [min_z, max_z] = details::parseReconstructedVolumeBoundary(min_z_, max_z_, row_count);

    uint32_t s_size = slice_size_.value_or(col_count);
#if defined(SLICE_EXPANSION)
    s_size = expandDataSize(s_size, SLICE_EXPANSION);
#endif

    uint32_t p_size = volume_size_.value_or(128);
    float half_slice_height = 0.5f * (max_z - min_z) / p_size;
    float z0 = 0.5f * (max_z + min_z);

    ProjectionGeometry proj_geom {
            beam_shape_, col_count, row_count, pixel_width_, pixel_height_, source2origin_, origin2detector_,
            defaultAngles(angle_count_, angle_range_ == AngleRange::HALF ? 1.f : 2.f)
    };
    VolumeGeometry slice_geom {
            s_size, s_size, 1, min_x, max_x, min_y, max_y, z0 - half_slice_height, z0 + half_slice_height
    };
    VolumeGeometry volume_geom {
            p_size, p_size, p_size, min_x, max_x, min_y, max_y, min_y, max_y
    };

    double_buffering_ = scan_mode_ == rpc::ScanMode_Mode_DYNAMIC;
    recon_.reset();
    recon_ = recon_factory_->create(proj_geom, slice_geom, volume_geom, double_buffering_);

    slice_mediator_->resize({slice_geom.col_count, slice_geom.row_count});

    auto shape = volume_proxy_->shape();
    if (shape[0] != volume_geom.col_count || shape[1] != volume_geom.row_count || shape[2] != volume_geom.slice_count) {
        volume_proxy_->reshapeBuffer({volume_geom.col_count, volume_geom.row_count, volume_geom.slice_count});
    }
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

void Application::pushDark(Projection<>&& proj) {
    darks_.emplace_back(std::move(proj.data));
    if (darks_.size() > k_MAX_NUM_DARKS) {
        spdlog::warn("Maximum number of dark images received. Data ignored!");
    }
}

void Application::pushFlat(Projection<>&& proj) {
    flats_.emplace_back(std::move(proj.data));
    if (flats_.size() > k_MAX_NUM_FLATS) {
        spdlog::warn("Maximum number of flat images received. Data ignored!");
    }
}

void Application::pushProjection(const Projection<>& proj) {
    if (pipeline_wait_on_slowness_ && raw_buffer_.isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    raw_buffer_.fill<RawDtype>(proj.index,
                               reinterpret_cast<const char*>(proj.data.data()),
                               proj.data.shape(),
                               {imgproc_params_.downsampling_row, imgproc_params_.downsampling_col});
}

} // namespace recastx::recon
