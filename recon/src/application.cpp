/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <chrono>
#include <complex>
#include <numeric>

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "recon/application.hpp"
#include "recon/encoder.hpp"
#include "recon/phase.hpp"
#include "recon/preprocessing.hpp"
#include "recon/rpc_server.hpp"
#include "recon/slice_mediator.hpp"
#include "common/scoped_timer.hpp"

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
       slice_mediator_(new SliceMediator()),
       imgproc_params_(imageproc_params),
       server_state_(ServerState_State_INIT),
       scan_mode_(ScanMode_Mode_DISCRETE),
       scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL),
       daq_client_(daq_client),
       ramp_filter_factory_(ramp_filter_factory),
       recon_factory_(recon_factory),
       rpc_server_(new RpcServer(rpc_config.port, this)) {}

Application::~Application() { 
    running_ = false; 
} 

void Application::init() {
    uint32_t downsampling_col = imgproc_params_.downsampling_col;
    uint32_t downsampling_row = imgproc_params_.downsampling_row;
    size_t col_count = proj_geom_.col_count / downsampling_col;
    size_t row_count = proj_geom_.row_count / downsampling_row;

    maybeInitFlatFieldBuffer(row_count, col_count);

    maybeInitReconBuffer(col_count, row_count);

    initPaganin(col_count, row_count);
    
    initFilter(col_count, row_count);
    
    sino_initialized_ = false;
    gpu_buffer_index_ = 0;
    initReconstructor(col_count, row_count);

    spdlog::info("Initial parameters for real-time 3D tomographic reconstruction:");
    spdlog::info("- Number of required dark images: {}", flatfield_params_.num_darks);
    spdlog::info("- Number of required flat images: {}", flatfield_params_.num_flats);
    spdlog::info("- Number of projection images per tomogram: {}", proj_geom_.angles.size());
    spdlog::info("- Projection image size: {} ({}) x {} ({})", 
                 col_count, downsampling_col, row_count, downsampling_row);
    spdlog::info("- Ramp filter: {}", imgproc_params_.ramp_filter.name);
}

void Application::setProjectionGeometry(BeamShape beam_shape, size_t col_count, size_t row_count,
                                        float pixel_width, float pixel_height,
                                        float src2origin, float origin2det, size_t num_angles) {
    proj_geom_ = {beam_shape, col_count, row_count, pixel_width, pixel_height, 
                  src2origin, origin2det, defaultAngles(num_angles)};
}

void Application::setReconGeometry(std::optional<size_t> slice_size, std::optional<size_t> preview_size,
                                   std::optional<float> minx, std::optional<float> maxx, 
                                   std::optional<float> miny, std::optional<float> maxy, 
                                   std::optional<float> minz, std::optional<float> maxz) {
    slice_size_ = slice_size;
    preview_size_ = preview_size;
    min_x_ = minx;
    max_x_ = maxx;
    min_y_ = miny;
    max_y_ = maxy;
    min_z_ = minz;
    max_z_ = maxz;
    // initialization is delayed
}

void Application::pushDark(const Projection& proj) {
    maybeResetDarkAndFlatAcquisition();
    size_t n = darks_.push(static_cast<const char*>(proj.data.data()));
    if (n % 10 == 0) {
        spdlog::info("# of darks received: {}", n);
    }
}

void Application::pushFlat(const Projection& proj) {
    maybeResetDarkAndFlatAcquisition();
    size_t n = flats_.push(static_cast<const char*>(proj.data.data()));
    if (n % 10 == 0) {
        spdlog::info("# of flats received: {}", n);
    }
}

void Application::pushProjection(const Projection& proj) {
    if (!reciprocal_computed_) {
        if (darks_.empty() || flats_.empty()) {
            spdlog::warn("Send dark and flat images first! Projection ignored.");
            return;
        }

        if (!darks_.full()) {
            spdlog::warn("Computing reciprocal with less darks than expected.");
        }
        if (!flats_.full()) {
            spdlog::warn("Computing reciprocal with less flats than expected.");
        }

        spdlog::info("Computing reciprocal for flat field correction ...");
        
        {    
#if (VERBOSITY >= 2)
            ScopedTimer timer("Bench", "Computing reciprocal");
#endif
        
            auto [dark_avg, reciprocal] = computeReciprocal(darks_, flats_);

            spdlog::debug("Downsampling dark ... ");
            downsample(dark_avg, dark_avg_);
            spdlog::debug("Downsampling reciprocal ... ");
            downsample(reciprocal, reciprocal_);
        }
    
        reciprocal_computed_ = true;
        monitor_.resetTimer();
    }

    // TODO: compute the average on the fly instead of storing the data in the buffer
    raw_buffer_.fill<RawDtype>(
        static_cast<const char*>(proj.data.data()), proj.index, {proj.row_count, proj.col_count});
}

void Application::startAcquiring() {
    auto t = std::thread([&] {
        while (running_) {
            if (waitForAcquiring()) continue;

            auto item = daq_client_->next();
            if (!item.has_value()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            auto& data = item.value();
            static size_t counter = 0;
            switch(data.type) {
                case ProjectionType::PROJECTION: {
                    pushProjection(data);
                    monitor_.addProjection();
                    if (counter++ % 10 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                    break;
                }
                case ProjectionType::DARK: {
                    pushDark(data);
                    monitor_.addDark();
                    break;
                }
                case ProjectionType::FLAT: {
                    pushFlat(data);
                    monitor_.addFlat();
                    break;
                }
                default:
                    break;
            }
        }
    });

    t.detach();
}

void Application::startPreprocessing() {
    auto t = std::thread([&] {
        oneapi::tbb::task_arena arena(imgproc_params_.num_threads);
        while (running_) {
            if (waitForProcessing()) continue;

            if (!raw_buffer_.fetch(100)) continue; 

            spdlog::info("Preprocessing projections ...");
            preprocessProjections(arena);
            spdlog::debug("Projection preprocessing finished!");
        }
    });

    t.detach();
}

void Application::startUploading() {
    auto t = std::thread([&] {
        while (running_) {
            if(waitForProcessing()) continue;

            if (!sino_buffer_.fetch(100)) continue;

            spdlog::info("Uploading sinograms to GPU ...");
            size_t chunk_size = sino_buffer_.front().shape()[0];
            {
                if (scan_mode_ == ScanMode_Mode_DISCRETE) {
                    recon_->uploadSinograms(1 - gpu_buffer_index_, sino_buffer_.front().data(), chunk_size);
                    std::lock_guard<std::mutex> lck(gpu_mtx_);
                    gpu_buffer_index_ = 1 - gpu_buffer_index_;
                    sino_uploaded_ = true;
                } else {
                    std::lock_guard<std::mutex> lck(gpu_mtx_);
                    sino_uploaded_ = true;
                    recon_->uploadSinograms(gpu_buffer_index_, sino_buffer_.front().data(), chunk_size);
                }

                sino_initialized_ = true;
                spdlog::debug("Sinogram uploaded!");
            }

            gpu_cv_.notify_one();
        }
    });

    t.detach();
}

void Application::startReconstructing() {

    auto t = std::thread([&] {
        while (running_) {          
            {
                std::unique_lock<std::mutex> lck(gpu_mtx_);
                if (gpu_cv_.wait_for(lck, 10ms, [&] { return sino_uploaded_; })) {
                    spdlog::info("Reconstructing preview and slices ...");
                    recon_->reconstructPreview(gpu_buffer_index_, preview_buffer_.back());
                } else {
                    if (waitForSinoInitialization()) continue;
                    slice_mediator_->reconOnDemand(recon_.get(), gpu_buffer_index_);
                    continue;
                }

                slice_mediator_->reconAll(recon_.get(), gpu_buffer_index_);

                sino_uploaded_ = false;
            }
            
            spdlog::debug("Preview and slices reconstructed!");
            monitor_.addTomogram();

            preview_buffer_.prepare();
        }

    });

    t.detach();
}

void Application::spin(bool auto_processing) {
    startAcquiring();    
    startPreprocessing();
    startUploading();
    startReconstructing();

    daq_client_->start();
    rpc_server_->start();

    if (auto_processing) {
        onStateChanged(ServerState_State::ServerState_State_PROCESSING);
    }

    // TODO: start the event loop in the main thread
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

void Application::setSlice(size_t timestamp, const Orientation& orientation) {
    slice_mediator_->update(timestamp, orientation);
}

void Application::setScanMode(ScanMode_Mode mode, uint32_t update_interval) {
    scan_mode_ = mode;
    scan_update_interval_ = update_interval;
    
    if (mode == ScanMode_Mode_DISCRETE) {
        spdlog::debug("Set scan mode: {}", static_cast<int>(mode));
    } else {
        spdlog::debug("Set scan mode: {}, update interval {}", 
                      static_cast<int>(mode), update_interval);
    }
}

void Application::onStateChanged(ServerState_State state) {
    if (server_state_ == state) {
        spdlog::debug("Server already in state: {}", static_cast<int>(state));
        return;
    }

    if (state == ServerState_State::ServerState_State_PROCESSING) {
        onStartProcessing();
    } else if (state == ServerState_State::ServerState_State_ACQUIRING) {
        onStartAcquiring();
    } else if (state == ServerState_State::ServerState_State_READY) {
        onStopProcessing();
    }
    
    server_state_ = state;
}

std::optional<rpc::ProjectionData> Application::projectionData(int timeout) {
    std::vector<float> vec(raw_buffer_.shape()[1] * raw_buffer_.shape()[2]);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    auto f = [&] { return dist(gen); };
    std::generate(vec.begin(), vec.end(), f);
    
    return createProjectionDataPacket(vec, raw_buffer_.shape()[1], raw_buffer_.shape()[2]);
}

std::optional<ReconData> Application::previewData(int timeout) { 
    if (preview_buffer_.fetch(timeout)) {
        auto& data = preview_buffer_.front();
        auto [x, y, z] = data.shape();
        return createVolumeDataPacket(data, x, y, z);
    }
    return std::nullopt;
}

std::vector<ReconData> Application::sliceData(int timeout) {
    std::vector<ReconData> ret;
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

std::vector<ReconData> Application::onDemandSliceData(int timeout) {
    std::vector<ReconData> ret;
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

void Application::preprocessProjections(oneapi::tbb::task_arena& arena) {

    auto& shape = raw_buffer_.shape();
    auto [chunk_size, row_count, col_count] = shape;
    size_t num_pixels = row_count * col_count;

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Preprocessing projections");
#endif

    auto projs = raw_buffer_.front().data();

    using namespace oneapi;
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(chunk_size)),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                float* p = &projs[i * num_pixels];

                flatField(p, num_pixels, dark_avg_, reciprocal_);

                if (paganin_) paganin_->apply(p, i % imgproc_params_.num_threads);
                else
                    negativeLog(p, num_pixels);

                ramp_filter_->apply(p, tbb::this_task_arena::current_thread_index());

                // TODO: Add FDK scaler for cone beam
            }
        });
    });

    // (projection_id, rows, cols) -> (rows, projection_id, cols).
    auto& sinos = sino_buffer_.back();
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, row_count),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                for (size_t j = 0; j < chunk_size; ++j) {
                    for (size_t k = 0; k < col_count; ++k) {
                        sinos[i * chunk_size * col_count + j * col_count + k] = 
                            projs[j * col_count * row_count + i * col_count + k];
                    }
                }
            }
        });
    });

    sino_buffer_.prepare();
}

void Application::onStartProcessing() {
    init();
    daq_client_->startAcquiring();

    spdlog::info("Start acquiring and processing data:");

    if (scan_mode_ == ScanMode_Mode_CONTINUOUS) {
        spdlog::info("- Scan mode: continuous");
        spdlog::info("- Update interval: {}", scan_update_interval_);
    } else if (scan_mode_ == ScanMode_Mode_DISCRETE) {
        spdlog::info("- Scan mode: discrete");
    }

    monitor_ = Monitor(proj_geom_.row_count * proj_geom_.col_count * proj_geom_.angles.size() 
                       * sizeof(RawDtype));
}

void Application::onStopProcessing() {
    daq_client_->stopAcquiring();
    spdlog::info("Stop acquiring and processing data");

    monitor_.summarize();
}

void Application::onStartAcquiring() {
    init();
    daq_client_->startAcquiring();
    spdlog::info("Start acquiring data");
}

void Application::initPaganin(size_t col_count, size_t row_count) {
    if (paganin_cfg_.has_value()) {
        auto& cfg = paganin_cfg_.value();
        paganin_ = std::make_unique<Paganin>(
            cfg.pixel_size, cfg.lambda, cfg.delta, cfg.beta, cfg.distance, 
            &raw_buffer_.front()[0], col_count, row_count);
    }
}

void Application::initFilter(size_t col_count, size_t row_count) {
    ramp_filter_ = ramp_filter_factory_->create(imgproc_params_.ramp_filter.name, &raw_buffer_.front()[0], 
                                                col_count, row_count, imgproc_params_.num_threads);
}

void Application::initReconstructor(size_t col_count, size_t row_count) {
    auto [min_x, max_x] = details::parseReconstructedVolumeBoundary(min_x_, max_x_, col_count);
    auto [min_y, max_y] = details::parseReconstructedVolumeBoundary(min_y_, max_y_, col_count);
    auto [min_z, max_z] = details::parseReconstructedVolumeBoundary(min_z_, max_z_, row_count);
    
    size_t s_size = slice_size_.value_or(col_count);
    size_t p_size = preview_size_.value_or(128);
    float half_slice_height = 0.5f * (max_z - min_z) / p_size;
    float z0 = 0.5f * (max_z + min_z);

    slice_geom_ = {s_size, s_size, 1, min_x, max_x, min_y, max_y, z0 - half_slice_height, z0 + half_slice_height};
    preview_geom_ = {p_size, p_size, p_size, min_x, max_x, min_y, max_y, min_z, max_z};

    preview_buffer_.resize({preview_geom_.col_count, preview_geom_.row_count, preview_geom_.slice_count});
    slice_mediator_->resize({slice_geom_.col_count, slice_geom_.row_count});

    bool double_buffering = scan_mode_ == ScanMode_Mode_DISCRETE;
    recon_ = recon_factory_->create(
        col_count, row_count, proj_geom_, slice_geom_, preview_geom_, double_buffering);
}

void Application::maybeInitFlatFieldBuffer(size_t row_count, size_t col_count) {
    // store original darks and flats
    size_t num_darks = flatfield_params_.num_darks;
    size_t num_flats = flatfield_params_.num_flats;
    size_t row_count_orig = proj_geom_.row_count;
    size_t col_count_orig = proj_geom_.col_count;

    auto shape_d = darks_.shape();
    if (shape_d[0] != num_darks || shape_d[1] != row_count_orig || shape_d[2] != col_count_orig) {
        darks_.resize({num_darks, row_count_orig, col_count_orig});
        spdlog::debug("Dark image buffer resized");
    }

    auto shape_f = flats_.shape();
    if (shape_f[0] != num_flats || shape_f[1] != row_count_orig || shape_f[2] != col_count_orig) {
        flats_.resize({num_flats, row_count_orig, col_count_orig});
        spdlog::debug("Flat image buffer resized");
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
    size_t chunk_size = (scan_mode_ == ScanMode_Mode_CONTINUOUS 
                         ? scan_update_interval_ : proj_geom_.angles.size());
    auto shape = raw_buffer_.shape();
    if (shape[0] != chunk_size || shape[1] != row_count || shape[2] != col_count) {
        raw_buffer_.resize({chunk_size, row_count, col_count});
        sino_buffer_.resize({chunk_size, row_count, col_count});
        spdlog::debug("Reconstruction buffers resized");
    }
    raw_buffer_.reset();
}

void Application::maybeResetDarkAndFlatAcquisition() {
    if (reciprocal_computed_) {
        raw_buffer_.reset();
        darks_.reset();
        flats_.reset();
        reciprocal_computed_ = false;
    }
}

} // namespace recastx::recon
