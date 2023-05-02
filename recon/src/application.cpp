#include <chrono>
#include <complex>
#include <numeric>
using namespace std::chrono_literals;

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "recon/application.hpp"
#include "recon/daq_client.hpp"
#include "recon/encoder.hpp"
#include "recon/filter.hpp"
#include "recon/phase.hpp"
#include "recon/preprocessing.hpp"
#include "recon/reconstructor.hpp"
#include "recon/zmq_server.hpp"
#include "common/scoped_timer.hpp"

namespace recastx::recon {

using namespace std::string_literals;

Application::Application(size_t raw_buffer_size, int num_threads, 
                         const DaqClientConfig& client_config, 
                         const ZmqServerConfig& server_config)
     : raw_buffer_(raw_buffer_size), 
       num_threads_(num_threads),
       daq_client_("tcp://"s + client_config.hostname + ":"s + std::to_string(client_config.port),
                   client_config.socket_type,
                   this),
       data_server_(server_config.data_port, this),
       msg_server_(server_config.message_port, this) {}

Application::~Application() { 
    running_ = false; 
} 

void Application::init() {
    if (initialized_) return;
    
    uint32_t downsampling_col = imageproc_params_.downsampling_col;
    uint32_t downsampling_row = imageproc_params_.downsampling_row;
    size_t col_count = proj_geom_.col_count / downsampling_col;
    size_t row_count = proj_geom_.row_count / downsampling_row;

    size_t num_darks = flatfield_params_.num_darks;
    size_t num_flats = flatfield_params_.num_flats;
    size_t num_angles = proj_geom_.angles.size();

    // original darks and flats are stored
    darks_.reshape({num_darks, proj_geom_.row_count, proj_geom_.col_count});
    flats_.reshape({num_flats, proj_geom_.row_count, proj_geom_.col_count});

    dark_avg_.reshape({row_count, col_count});
    reciprocal_.reshape({row_count, col_count});
    reciprocal_computed_ = false;

    raw_buffer_.reshape({num_angles, row_count, col_count});
    sino_buffer_.reshape({num_angles, row_count, col_count});

    initPaganin(col_count, row_count);
    initFilter(col_count, row_count);
    initReconstructor(col_count, row_count);

    initialized_ = true;
    spdlog::info("Initial parameters for real-time 3D tomographic reconstruction:");
    spdlog::info("- Number of required dark images: {}", num_darks);
    spdlog::info("- Number of required flat images: {}", num_flats);
    spdlog::info("- Number of projection images per tomogram: {}", num_angles);
    spdlog::info("- Projection image size: {} ({}) x {} ({})", 
                 col_count, downsampling_col, row_count, downsampling_row);
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

void Application::startPreprocessing() {
    auto t = std::thread([&] {
        oneapi::tbb::task_arena arena(num_threads_);
        while (running_) {
            if (!raw_buffer_.fetch(100)) continue; 
            if (state_ != StatePacket_State::StatePacket_State_PROCESSING) continue;

            spdlog::info("Processing projections ...");
            processProjections(arena);
        }
    });

    t.detach();
}

void Application::startUploading() {
    auto t = std::thread([&] {

        size_t num_angles = proj_geom_.angles.size();
        while (running_) {
            if (!sino_buffer_.fetch(100)) continue;
            if (state_ != StatePacket_State::StatePacket_State_PROCESSING) continue;

            spdlog::info("Uploading sinograms to GPU ...");
            recon_->uploadSinograms(
                1 - gpu_buffer_index_, sino_buffer_.front().data(), 0, num_angles - 1);

            {
                std::lock_guard<std::mutex> lck(gpu_mutex_);
                sino_uploaded_ = true;
                gpu_buffer_index_ = 1 - gpu_buffer_index_;
            }
            gpu_cv_.notify_one();
        }
    });

    t.detach();
}

void Application::startReconstructing() {

    auto t = std::thread([&] {

#if (VERBOSITY >= 1)
        const float data_size = sino_buffer_.front().shape()[0] * sizeof(RawDtype) / (1024.f * 1024.f); // in MB
        auto start = std::chrono::steady_clock::now();
        size_t count = 0;
        float total_duration = 0.f;
#endif

        while (running_) {
            {
                std::unique_lock<std::mutex> lck(gpu_mutex_);
                if (gpu_cv_.wait_for(lck, 10ms, [&] { return sino_uploaded_; })) {
                    if (state_ != StatePacket_State::StatePacket_State_PROCESSING) continue;
                    recon_->reconstructPreview(gpu_buffer_index_, preview_buffer_.back());
                } else {
                    slice_mediator_.reconOnDemand(recon_.get(), gpu_buffer_index_);
                    continue;
                }

                slice_mediator_.reconAll(recon_.get(), gpu_buffer_index_);

                sino_uploaded_ = false;
            }
            
            preview_buffer_.prepare();
 
#if (VERBOSITY >= 1)
            // The throughtput is measured in the last step of the pipeline because
            // data could be dropped beforehand.
            float duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() -  start).count();
            start = std::chrono::steady_clock::now();

            float tp = data_size * 1000000 / duration;
            ++count;
            float tp_avg;
            if (count == 1) tp_avg = tp;
            else {
                // skip the first group since the duration includes the time for initialization as well as
                // processing darks and flats
                total_duration += duration;
                tp_avg = data_size * (count - 1) * 1000000 / total_duration;
            }
            spdlog::info("[bench] Throughput (reconstruction) (MB/s). "
                         "Current: {:.1f}, averaged: {:.1f} ({})", tp, tp_avg, count);
#endif

        }

    });

    t.detach();
}

void Application::runForEver() {
    startPreprocessing();
    startUploading();
    startReconstructing();

    daq_client_.start();
    data_server_.start();
    msg_server_.start();

    // TODO: start the event loop in the main thread
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Application::pushProjection(
        ProjectionType k, size_t proj_idx, size_t num_rows, size_t num_cols, const char* data) {

    switch (k) {
        case ProjectionType::projection: {
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

                    spdlog::info("Downsampling dark ... ");
                    downsample(dark_avg, dark_avg_);
                    spdlog::info("Downsampling reciprocal ... ");
                    downsample(reciprocal, reciprocal_);
                }
                reciprocal_computed_ = true;
            }

            size_t num_angles = proj_geom_.angles.size();
            // TODO: compute the average on the fly instead of storing the data in the buffer
            raw_buffer_.fill<RawDtype>(data, proj_idx / num_angles, proj_idx % num_angles, {num_rows, num_cols});
            break;
        }
        case ProjectionType::dark: {
            maybeResetDarkAndFlatAcquisition();
            spdlog::info("Received {} darks in total.", darks_.push(data));
            break;
        }
        case ProjectionType::flat: {
            maybeResetDarkAndFlatAcquisition();
            spdlog::info("Received {} flats in total.", flats_.push(data));
            break;
        }
        default:
            break;
    }
}

void Application::setSlice(size_t timestamp, const Orientation& orientation) {
    slice_mediator_.update(timestamp, orientation);
}

void Application::onStateChanged(StatePacket_State state) {
    if (state_ == state) return;

    state_ = state;
    if (state == StatePacket_State::StatePacket_State_PROCESSING) {
        init();
        daq_client_.startAcquiring();
        spdlog::info("Start acquiring and processing ...");
    } else if (state == StatePacket_State::StatePacket_State_ACQUIRING) {
        daq_client_.startAcquiring();
        spdlog::info("Start acquiring ...");
    } else if (state == StatePacket_State::StatePacket_State_READY) {
        daq_client_.stopAcquiring();
        spdlog::info("Stop acquiring and processing ...");
    }
}

std::optional<ReconDataPacket> Application::previewDataPacket(int timeout) { 
    if (preview_buffer_.fetch(timeout)) {
        auto& data = preview_buffer_.front();
        auto [x, y, z] = data.shape();
        return createVolumeDataPacket(data, x, y, z);
    }
    return std::nullopt;
}

std::vector<ReconDataPacket> Application::sliceDataPackets(int timeout) {
    std::vector<ReconDataPacket> ret;
    auto& buffer = slice_mediator_.allSlices();
    if (buffer.fetch(timeout)) {
        for (auto& [k, slice] : buffer.front()) {
            auto& data = std::get<2>(slice);
            auto [x, y] = data.shape();
            ret.emplace_back(createSliceDataPacket(data, x, y, std::get<1>(slice)));
        }
    }
    return ret;
}

std::vector<ReconDataPacket> Application::onDemandSliceDataPackets(int timeout) {
    std::vector<ReconDataPacket> ret;
    auto& buffer = slice_mediator_.onDemandSlices();
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

void Application::processProjections(oneapi::tbb::task_arena& arena) {

    auto& shape = raw_buffer_.shape();
    auto [num_angles, row_count, col_count] = shape;
    size_t num_pixels = row_count * col_count;

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Processing projections");
#endif

    auto projs = raw_buffer_.front().data();

    using namespace oneapi;
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(num_angles)),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                float* p = &projs[i * num_pixels];

                flatField(p, num_pixels, dark_avg_, reciprocal_);

                if (paganin_) paganin_->apply(p, i % num_threads_);
                else
                    negativeLog(p, num_pixels);

                filter_->apply(p, tbb::this_task_arena::current_thread_index());

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
                for (size_t j = 0; j < num_angles; ++j) {
                    for (size_t k = 0; k < col_count; ++k) {
                        sinos[i * num_angles * col_count + j * col_count + k] = 
                            projs[j * col_count * row_count + i * col_count + k];
                    }
                }
            }
        });
    });

    sino_buffer_.prepare();
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
    filter_ = std::make_unique<Filter>(
        filter_cfg_.name, filter_cfg_.gaussian_lowpass_filter, 
        &raw_buffer_.front()[0], col_count, row_count, num_threads_);
}

void Application::initReconstructor(size_t col_count, size_t row_count) {
    auto [min_x, max_x] = details::parseReconstructedVolumeBoundary(min_x_, max_x_, col_count);
    auto [min_y, max_y] = details::parseReconstructedVolumeBoundary(min_y_, max_y_, col_count);
    auto [min_z, max_z] = details::parseReconstructedVolumeBoundary(min_z_, max_z_, row_count);
    
    size_t slice_size = slice_size_.value_or(col_count);
    size_t preview_size = preview_size_.value_or(128);

    float half_slice_height = 0.5f * (max_z - min_z) / preview_size;
    float z0 = 0.5f * (max_z + min_z);

    slice_geom_ = {slice_size, slice_size, 1, min_x, max_x, min_y, max_y, z0 - half_slice_height, z0 + half_slice_height};
    preview_geom_ = {preview_size, preview_size, preview_size, min_x, max_x, min_y, max_y, min_z, max_z};

    preview_buffer_.reshape({preview_geom_.col_count, preview_geom_.row_count, preview_geom_.slice_count});
    slice_mediator_.reshape({slice_geom_.col_count, slice_geom_.row_count});
    if (proj_geom_.beam_shape == BeamShape::CONE) {
        recon_ = std::make_unique<ConeBeamReconstructor>(col_count, row_count, proj_geom_, slice_geom_, preview_geom_);
    } else {
        recon_ = std::make_unique<ParallelBeamReconstructor>(col_count, row_count, proj_geom_, slice_geom_, preview_geom_);
    }
}

} // namespace recastx::recon
