#include <complex>
#include <numeric>

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "slicerecon/reconstructor.hpp"
#include "slicerecon/utils.hpp"


namespace slicerecon {

Reconstructor::Reconstructor(int rows, int cols, int num_threads)
     : rows_(rows),
       cols_(cols),
       pixels_(rows * cols),
       num_threads_(num_threads), 
       arena_(num_threads) {}

Reconstructor::~Reconstructor() = default;

void Reconstructor::initialize(int num_darks, 
                               int num_flats, 
                               int num_projections,
                               int preview_size) {
    num_darks_ = num_darks;
    num_flats_ = num_flats;
    num_projections_ = num_projections;
    preview_size_ = preview_size;

    all_darks_.resize(pixels_ * num_darks);
    all_flats_.resize(pixels_ * num_flats);
    dark_avg_.resize(pixels_);
    reciprocal_.resize(pixels_, 1.0f);

    buffer_size_ = num_projections;
    buffer_.initialize((size_t)buffer_size_, pixels_);
    sino_buffer_.initialize((size_t)buffer_size_ * pixels_);
    preview_buffer_.initialize(preview_size * preview_size * preview_size);

    initialized_ = true;
    spdlog::info("Reconstructor initialized:");
    spdlog::info("- Number of dark images: {}", num_darks_);
    spdlog::info("- Number of flat images: {}", num_flats_);
    spdlog::info("- Number of projection images: {}", num_projections_);
}

void Reconstructor::initPaganin(float pixel_size, 
                                float lambda, 
                                float delta, 
                                float beta, 
                                float distance) {
    if (!initialized_) throw std::runtime_error("Reconstructor not initialized!");

    paganin_ = std::make_unique<Paganin>(
        pixel_size, lambda, delta, beta, distance, &buffer_.front()[0], rows_, cols_);
}

void Reconstructor::initFilter(const std::string& name, bool gaussian_pass) {
    if (!initialized_) throw std::runtime_error("Reconstructor not initialized!");

    filter_ = std::make_unique<Filter>(
        name, gaussian_pass, &buffer_.front()[0], rows_, cols_, num_threads_);
}

void Reconstructor::setSolver(std::unique_ptr<Solver>&& solver) {
    solver_ = std::move(solver);
}

void Reconstructor::start() {
    gpu_upload_thread_ = std::thread([&] {
        while (true) {
            sino_buffer_.fetch();

            spdlog::info("Uploading sinograms to GPU ...");
            solver_->uploadSinograms(
                1 - gpu_buffer_index_, sino_buffer_.front(), 0, buffer_size_ - 1);

            {
                std::lock_guard<std::mutex> lck(gpu_mutex_);
                sino_uploaded_ = true;
                gpu_buffer_index_ = 1 - gpu_buffer_index_;
            }
            gpu_cv_.notify_one();
        }
    });

    gpu_recon_thread_ = std::thread([&] {
        while (true) {
            {
                std::unique_lock<std::mutex> lck(gpu_mutex_);
                gpu_cv_.wait(lck, [&] { return sino_uploaded_; });
                solver_->reconstructPreview(preview_buffer_.back(), gpu_buffer_index_);
                sino_uploaded_ = false;
            }
            preview_buffer_.prepare();
        }
    });

    gpu_upload_thread_.detach();
    gpu_recon_thread_.detach();
}

void Reconstructor::pushProjection(ProjectionType k, 
                                   int32_t proj_idx, 
                                   const std::array<int32_t, 2>& shape, 
                                   const char* data) {

    if (shape[0] != rows_ || shape[1] != cols_) {
        spdlog::error("Received projection with wrong shape. Actual: {} x {}, expected: {} x {}", 
                      shape[0], shape[1], rows_, cols_);
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
                utils::computeReciprocal(all_darks_, all_flats_, pixels_, reciprocal_, dark_avg_);

                reciprocal_computed_ = true;
                received_darks_ = 0;
                received_flats_ = 0;
            }

            if (!reciprocal_computed_) {
                spdlog::warn("Send dark and flat images first! Projection ignored.");
                return;
            }

            // TODO: compute the average on the fly instead of storing the data in the buffer
            buffer_.fill<RawDtype>(data, proj_idx / buffer_size_, proj_idx % buffer_size_, pixels_);
            if (buffer_.full()) {
                spdlog::info("Processing projections ...");
                processProjections();
            }

            break;
        }
        case ProjectionType::dark: {
            reciprocal_computed_ = false;
            if (received_darks_ == num_darks_) {
                spdlog::warn("Received more darks than expected. New dark ignored!");
                return;
            }
            memcpy(&all_darks_[received_darks_ * pixels_], data, sizeof(RawDtype) * pixels_);
            ++received_darks_;
            spdlog::info("Received dark No. {0:d}", received_darks_);
            break;
        }
        case ProjectionType::flat: {
            reciprocal_computed_ = false;
            if (received_flats_ == num_flats_) {
                spdlog::warn("Received more flats than expected. New flat ignored!");
                return;
            }
            memcpy(&all_flats_[received_flats_ * pixels_], data, sizeof(RawDtype) * pixels_);
            ++received_flats_;
            spdlog::info("Received flat No. {0:d}", received_flats_);
            break;
        }
        default:
            break;
    }
}

slice_data Reconstructor::reconstructSlice(orientation x) {
    std::lock_guard<std::mutex> lck(gpu_mutex_);
    return solver_->reconstructSlice(x, gpu_buffer_index_);
}

const std::vector<float>& Reconstructor::previewData() { 
    preview_buffer_.fetch();
    return preview_buffer_.front(); 
}

int Reconstructor::previewSize() const { return preview_size_; }

int Reconstructor::bufferSize() const { return buffer_size_; }

void Reconstructor::processProjections() {
#if defined(WITH_MONITOR)
    auto start = std::chrono::steady_clock::now();
#endif

    auto projs = buffer_.back().data();
    using namespace oneapi;
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, buffer_size_),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                float* p = &projs[i * pixels_];

                utils::flatField(p, pixels_, dark_avg_, reciprocal_);

                if (paganin_) paganin_->apply(p, i % num_threads_);
                else
                    utils::negativeLog(p, pixels_);

                filter_->apply(p, tbb::this_task_arena::current_thread_index());

                // TODO: Add FDK scaler for cone beam
            }
        });
    });

    // (projection_id, rows, cols) -> (rows, projection_id, cols).
    auto& sinos = sino_buffer_.back();
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, rows_),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                for (int j = 0; j < buffer_size_; ++j) {
                    for (int k = 0; k < cols_; ++k) {
                        sinos[i * buffer_size_ * cols_ + j * cols_ + k] = 
                            projs[j * cols_ * rows_ + i * cols_ + k];
                    }
                }
            }
        });
    });

    buffer_.swap();
    sino_buffer_.prepare();

#if defined(WITH_MONITOR)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Processing projections took {} ms", duration / 1000);
#endif
}

const std::vector<RawDtype>& Reconstructor::darks() const { return all_darks_; }
const std::vector<RawDtype>& Reconstructor::flats() const { return all_flats_; }
const Buffer2<float>& Reconstructor::buffer() const { return buffer_; }
const SimpleBuffer3<float>& Reconstructor::sinoBuffer() const { return sino_buffer_; }

} // namespace slicerecon
