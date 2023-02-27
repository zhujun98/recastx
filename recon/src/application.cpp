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

namespace tomcat::recon {

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

Application::~Application() = default;

void Application::init(size_t num_rows, size_t num_cols, size_t num_angles, 
                       size_t num_darks, size_t num_flats,
                       size_t downsample_row, size_t downsample_col) {
    num_rows_ = num_rows;
    num_cols_ = num_cols;
    num_angles_ = num_angles;
    downsample_row_ = downsample_row;
    downsample_col_ = downsample_col;

    darks_.reshape({num_darks, num_rows_, num_cols_});
    flats_.reshape({num_flats, num_rows_, num_cols_});
    dark_avg_.reshape({num_rows_, num_cols_});
    reciprocal_.reshape({num_rows_, num_cols_});

    raw_buffer_.resize({num_angles, num_rows_, num_cols_});
    sino_buffer_.reshape({num_angles, num_rows_, num_cols_});

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
    preview_buffer_.reshape({preview_geom.col_count, preview_geom.row_count, preview_geom.slice_count});
    slice_mediator_.reshape({slice_geom.col_count, slice_geom.row_count});
    if (cone_beam) {
        recon_ = std::make_unique<ConeBeamReconstructor>(proj_geom, slice_geom, preview_geom);
    } else {
        recon_ = std::make_unique<ParallelBeamReconstructor>(proj_geom, slice_geom, preview_geom);
    }
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
                1 - gpu_buffer_index_, sino_buffer_.front().data(), 0, num_angles_ - 1);

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
        const float data_size = sino_buffer_.front().shape()[0] * sizeof(RawDtype) / (1024.f * 1024.f); // in MB
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

    recon_thread_.detach();
}

void Application::runForEver() {
    startPreprocessing();
    startUploading();
    startReconstructing();

    daq_client_.start();
    data_server_.start();
    msg_server_.start();

    // TODO: start the event loop in the main thread
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Application::pushProjection(
        ProjectionType k, size_t proj_idx, size_t num_rows, size_t num_cols, const char* data) {


    switch (k) {
        case ProjectionType::projection: {
            if (!darks_.empty() && !flats_.empty()) {
                if (!darks_.full()) {
                    spdlog::warn("Computing reciprocal with less darks than expected.");
                }
                if (!flats_.full()) {
                    spdlog::warn("Computing reciprocal with less flats than expected.");
                }

                spdlog::info("Computing reciprocal for flat fielding ...");
                computeReciprocal(darks_, flats_, reciprocal_, dark_avg_);
                raw_buffer_.reset();

#if (VERBOSITY >= 1)
                spdlog::info("Memory buffer reset!");
#endif

                reciprocal_computed_ = true;
                darks_.reset();
                flats_.reset();
            }

            if (!reciprocal_computed_) {
                spdlog::warn("Send dark and flat images first! Projection ignored.");
                return;
            }

            // TODO: compute the average on the fly instead of storing the data in the buffer
            raw_buffer_.fill<RawDtype>(data, proj_idx / num_angles_, proj_idx % num_angles_, 
                                       {num_rows, num_cols}, {downsample_row_, downsample_col_});
            break;
        }
        case ProjectionType::dark: {
            reciprocal_computed_ = false;
            spdlog::info("Received {} darks in total.", 
                         darks_.push(data, {num_rows, num_cols}, {downsample_row_, downsample_col_}));
            break;
        }
        case ProjectionType::flat: {
            reciprocal_computed_ = false;
            spdlog::info("Received {} flats in total.", 
                         flats_.push(data, {num_rows, num_cols}, {downsample_row_, downsample_col_}));
            break;
        }
        default:
            break;
    }
}

void Application::setSlice(size_t timestamp, const Orientation& orientation) {
    slice_mediator_.update(timestamp, orientation);
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
