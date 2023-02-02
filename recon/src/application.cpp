#include <chrono>
#include <complex>
#include <numeric>
using namespace std::chrono_literals;

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "recon/application.hpp"
#include "recon/utils.hpp"
#include "recon/daq_client.hpp"
#include "recon/zmq_server.hpp"

namespace tomcat::recon {

using namespace std::string_literals;

Application::Application(size_t raw_buffer_size, int num_threads)
     : raw_buffer_(raw_buffer_size), num_threads_(num_threads) {}

Application::~Application() = default;

void Application::init(size_t num_cols, size_t num_rows, size_t num_angles, 
                       size_t num_darks, size_t num_flats) {
    num_cols_ = num_cols;
    num_rows_ = num_rows;
    num_pixels_ = num_rows_ * num_cols_;
    num_angles_ = num_angles;
    num_darks_ = num_darks;
    num_flats_ = num_flats;

    all_darks_.resize(num_pixels_ * num_darks);
    all_flats_.resize(num_pixels_ * num_flats);
    dark_avg_.resize(num_pixels_);
    reciprocal_.resize(num_pixels_, 1.0f);

    raw_buffer_.resize({num_angles, num_cols, num_rows});
    sino_buffer_.resize({num_angles, num_cols, num_rows});

    initialized_ = true;
    spdlog::info("Initial parameters for real-time 3D tomographic reconstruction:");
    spdlog::info("- Number of required dark images: {}", num_darks);
    spdlog::info("- Number of required flat images: {}", num_flats);
    spdlog::info("- Number of projection images per tomogram: {}", num_angles);
}

void Application::initPaganin(const PaganinConfig& config, int num_cols, int num_rows) {
    if (!initialized_) throw std::runtime_error("Application not initialized!");

    paganin_ = std::make_unique<Paganin>(
        config.pixel_size, config.lambda, config.delta, config.beta, config.distance, 
        &raw_buffer_.front()[0], num_cols, num_rows);
}

void Application::initFilter(const FilterConfig& config, int num_cols, int num_rows) {
    if (!initialized_) throw std::runtime_error("Application not initialized!");

    filter_ = std::make_unique<Filter>(
        config.name, config.gaussian_lowpass_filter, 
        &raw_buffer_.front()[0], num_cols, num_rows, num_threads_);
}

void Application::initReconstructor(bool cone_beam,
                                    const ProjectionGeometry& proj_geom,
                                    const VolumeGeometry& slice_geom,
                                    const VolumeGeometry& preview_geom) {
    preview_buffer_.resize({preview_geom.col_count, preview_geom.row_count, preview_geom.slice_count});
    slice_buffer_.resize({slice_geom.col_count, slice_geom.row_count});
    if (cone_beam) {
        recon_ = std::make_unique<ConeBeamReconstructor>(proj_geom, slice_geom, preview_geom);
    } else {
        recon_ = std::make_unique<ParallelBeamReconstructor>(proj_geom, slice_geom, preview_geom);
    }
}

void Application::initConnection(const DaqClientConfig& client_config, 
                                 const ZmqServerConfig& server_config) {
    daq_client_ = std::make_unique<DaqClient>(
        "tcp://"s + client_config.hostname + ":"s + std::to_string(client_config.port),
        client_config.socket_type,
        this);

    zmq_server_ = std::make_unique<ZmqServer>(
        server_config.data_port, server_config.message_port, this);
}

void Application::startPreprocessing() {
    preproc_thread_ = std::thread([&] {
        oneapi::tbb::task_arena arena(num_threads_);
        while (true) {
            raw_buffer_.fetch();
            spdlog::info("Processing projections ...");
            processProjections(arena);
        }
    });

    preproc_thread_.detach();
}

void Application::startUploading() {
    upload_thread_ = std::thread([&] {
        while (true) {
            sino_buffer_.fetch();

            spdlog::info("Uploading sinograms to GPU ...");
            recon_->uploadSinograms(
                1 - gpu_buffer_index_, sino_buffer_.front(), 0, num_angles_ - 1);

            {
                std::lock_guard<std::mutex> lck(gpu_mutex_);
                sino_uploaded_ = true;
                gpu_buffer_index_ = 1 - gpu_buffer_index_;
            }
            gpu_cv_.notify_one();
        }
    });

    upload_thread_.detach();
}

void Application::startReconstructing() {

    recon_thread_ = std::thread([&] {

#if (VERBOSITY >= 1)
        const float data_size = sino_buffer_.shape()[0] * sizeof(RawDtype) / (1024.f * 1024.f); // in MB
        auto start = std::chrono::steady_clock::now();
        size_t count = 0;
        float total_duration = 0.f;
#endif
 
        while (true) {
            {
                std::unique_lock<std::mutex> lck(gpu_mutex_);
                if (gpu_cv_.wait_for(lck, 10ms, [&] { return sino_uploaded_; })) {
                    recon_->reconstructPreview(gpu_buffer_index_, preview_buffer_.back());
                } else {
                    slice_buffer_.reconRequested(recon_.get(), gpu_buffer_index_);
                    continue;
                }

                slice_buffer_.reconAll(recon_.get(), gpu_buffer_index_);

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

    recon_thread_.detach();
}

void Application::runForEver() {
    startPreprocessing();
    startUploading();
    startReconstructing();

    daq_client_->start();
    zmq_server_->start();

    // TODO: start the event loop in the main thread
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Application::pushProjection(ProjectionType k, 
                                 size_t proj_idx, 
                                 const std::array<size_t, 2>& shape, 
                                 const char* data) {
    if (shape[0] != num_rows_ || shape[1] != num_cols_) {
        spdlog::error("Received projection with wrong shape. Actual: {} x {}, expected: {} x {}", 
                      shape[0], shape[1], num_rows_, num_cols_);
        throw std::runtime_error(
            "Received projection has a different shape than the one set by "
            "the acquisition geometry");
    }

    switch (k) {
        case ProjectionType::projection: {
            if (received_darks_ > 0 && received_flats_ > 0) {
                if (received_darks_ < num_darks_ || received_flats_ < num_flats_) {
                    spdlog::warn("Computing reciprocal with less darks and/or flats than expected. "
                                 "Received: {}/{}, Expected: {}/{} ...", 
                                 received_darks_, received_flats_, num_darks_, num_flats_);
                }

                spdlog::info("Computing reciprocal for flat fielding ...");
                utils::computeReciprocal(all_darks_, all_flats_, num_pixels_, reciprocal_, dark_avg_);
                raw_buffer_.reset();

#if (VERBOSITY >= 1)
                spdlog::info("Memory buffer reset!");
#endif

                reciprocal_computed_ = true;
                received_darks_ = 0;
                received_flats_ = 0;
            }

            if (!reciprocal_computed_) {
                spdlog::warn("Send dark and flat images first! Projection ignored.");
                return;
            }

            // TODO: compute the average on the fly instead of storing the data in the buffer
            raw_buffer_.fill<RawDtype>(data, proj_idx / num_angles_, proj_idx % num_angles_);
            break;
        }
        case ProjectionType::dark: {
            reciprocal_computed_ = false;
            ++received_darks_;
            if (received_darks_ == num_darks_) {
                spdlog::info("Received {} darks in total.", received_darks_);
            } else if (received_darks_ > num_darks_) {
                spdlog::warn("Received more darks than expected. New dark ignored!");
                return;
            }
            spdlog::info("Received: {}", received_darks_);
            memcpy(&all_darks_[(received_darks_ - 1) * num_pixels_], data, sizeof(RawDtype) * num_pixels_);
            break;
        }
        case ProjectionType::flat: {
            reciprocal_computed_ = false;
            ++received_flats_;
            if (received_flats_ == num_flats_) {
                spdlog::info("Received {} flats in total.", received_flats_);
            } else if (received_flats_ > num_flats_) {
                spdlog::warn("Received more flats than expected. New flat ignored!");
                return;
            }
            memcpy(&all_flats_[(received_flats_ - 1) * num_pixels_], data, sizeof(RawDtype) * num_pixels_);
            break;
        }
        default:
            break;
    }
}

void Application::setSlice(size_t timestamp, const Orientation& orientation) {
    slice_buffer_.insert(timestamp, orientation);
}

std::optional<VolumeDataPacket> Application::previewDataPacket() { 
    // - Do not block because slice request needs to be responsive
    // - If the number of the logical threads are more than the number of the physical threads, 
    //   the preview_data could always have value.
    if (preview_buffer_.fetch(0)) {
        auto [x, y, z] = preview_buffer_.shape();
        return VolumeDataPacket({x, y, z}, preview_buffer_.front());
    }
    return std::nullopt;
}

std::vector<SliceDataPacket> Application::sliceDataPackets() {
    std::vector<SliceDataPacket> ret;
    auto& buffer = slice_buffer_.slices();
    if (buffer.fetch(-1)) {
        auto [x, y] = buffer.shape();
        for (size_t i = 0; i < NUM_SLICES; ++i) {
            auto item = buffer.front().second[i];
            ret.emplace_back(SliceDataPacket(item.first, {x, y}, item.second));
        }
    }

    return ret;
}

std::vector<SliceDataPacket> Application::requestedSliceDataPackets() {
    std::vector<SliceDataPacket> ret;
    auto& buffer = slice_buffer_.requestedSlices();
    if (buffer.fetch(10)) {
        auto [x, y] = buffer.shape();
        for (auto sid : buffer.front().first) {
            auto item = buffer.front().second[sid];
            ret.emplace_back(SliceDataPacket(item.first, {x, y}, item.second));
        }
    }
    return ret;
}

size_t Application::bufferSize() const { return num_angles_; }

void Application::processProjections(oneapi::tbb::task_arena& arena) {

    auto& shape = raw_buffer_.shape();
    int num_angles = static_cast<int>(shape[0]);
    int num_pixels = static_cast<int>(shape[1] * shape[2]);

#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    auto projs = raw_buffer_.front().data();

    using namespace oneapi;
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, num_angles),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                float* p = &projs[i * num_pixels];

                utils::flatField(p, num_pixels, dark_avg_, reciprocal_);

                if (paganin_) paganin_->apply(p, i % num_threads_);
                else
                    utils::negativeLog(p, num_pixels_);

                filter_->apply(p, tbb::this_task_arena::current_thread_index());

                // TODO: Add FDK scaler for cone beam
            }
        });
    });

    // (projection_id, rows, cols) -> (rows, projection_id, cols).
    auto& sinos = sino_buffer_.back();
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, num_rows_),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                for (size_t j = 0; j < static_cast<size_t>(num_angles_); ++j) {
                    for (size_t k = 0; k < static_cast<size_t>(num_cols_); ++k) {
                        sinos[i * num_angles_ * num_cols_ + j * num_cols_ + k] = 
                            projs[j * num_cols_ * num_rows_ + i * num_cols_ + k];
                    }
                }
            }
        });
    });

    sino_buffer_.prepare();

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Processing projections took {} ms", duration / 1000);
#endif
}

} // tomcat::recon
