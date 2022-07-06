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

void Reconstructor::initialize(int num_darks, 
                               int num_flats, 
                               int num_projections,
                               int group_size,
                               int preview_size,
                               ReconstructMode recon_mode) {
    num_darks_ = num_darks;
    num_flats_ = num_flats;
    num_projections_ = num_projections;
    group_size_ = group_size;
    preview_size_ = preview_size;
    recon_mode_ = recon_mode;

    all_darks_.resize(pixels_ * num_darks);
    all_flats_.resize(pixels_ * num_flats);
    dark_avg_.resize(pixels_);
    reciprocal_.resize(pixels_, 1.0f);

    buffer_size_ = recon_mode == ReconstructMode::alternating ? num_projections : group_size;
    buffer_.resize((size_t)buffer_size_ * (size_t)pixels_);
    sino_buffer_.resize((size_t)buffer_size_ * (size_t)pixels_);
    preview_buffer_.initialize(preview_size * preview_size * preview_size);

    initialized_ = true;
    spdlog::info("Reconstructor initialized:");
    spdlog::info("- Number of dark images: {}", num_darks_);
    spdlog::info("- Number of flat images: {}", num_flats_);
    spdlog::info("- Number of projection images: {}", num_projections_);
    spdlog::info("- Group size: {}", group_size_);
    spdlog::info("- Mode: {}", recon_mode_ == ReconstructMode::alternating ? "alternating" : "continuous");
}

void Reconstructor::initPaganin(float pixel_size, 
                                float lambda, 
                                float delta, 
                                float beta, 
                                float distance) {
    if (!initialized_) throw std::runtime_error("Reconstructor not initialized!");

    paganin_ = std::make_unique<Paganin>(
        pixel_size, lambda, delta, beta, distance, &buffer_[0], rows_, cols_);
}

void Reconstructor::initFilter(const std::string& name, bool gaussian_pass) {
    if (!initialized_) throw std::runtime_error("Reconstructor not initialized!");

    filter_ = std::make_unique<Filter>(
        name, gaussian_pass, &buffer_[0], rows_, cols_, num_threads_);
}

void Reconstructor::setSolver(std::unique_ptr<Solver>&& solver) {
    solver_ = std::move(solver);
}

Reconstructor::~Reconstructor() = default;

void Reconstructor::pushProjection(ProjectionType k, 
                                   int32_t proj_idx, 
                                   const std::array<int32_t, 2>& shape, 
                                   char* data) {

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
            auto pos = data;
            int buffer_idx1 = proj_idx % buffer_size_;
            for (int i = 0, j = buffer_idx1 * pixels_; i < pixels_; ++i, ++j) {
                RawDtype v;
                memcpy(&v, pos, sizeof(RawDtype));
                pos += sizeof(RawDtype);
                buffer_[j] = static_cast<float>(v);
            }

            bool group_end_reached = buffer_idx1 % group_size_ == group_size_ - 1;
            bool buffer_end_reached = buffer_idx1 == buffer_size_ - 1;
            if (group_end_reached || buffer_end_reached) {
                auto buffer_idx0 = buffer_idx1 - (buffer_idx1 % group_size_);

                spdlog::info("Processing projection buffer between {0:d} and {1:d} ...", 
                             buffer_idx0, buffer_idx1);
                processProjections(buffer_idx0, buffer_idx1);
            }

            if (buffer_end_reached) {
                if (recon_mode_ == ReconstructMode::alternating) {
                    utils::projection2sino(
                        buffer_, sino_buffer_, rows_, cols_, 0, buffer_size_ - 1);
                    uploadSinoBuffer(0, buffer_size_ - 1);
                } else { // --continuous mode
                    auto begin_wrt_geom = (update_count_ * buffer_size_) % num_projections_;
                    auto end_wrt_geom = (begin_wrt_geom + buffer_size_ - 1) % num_projections_;

                    // we only have one buffer
                    if (end_wrt_geom > begin_wrt_geom) {
                        utils::projection2sino(buffer_, sino_buffer_, rows_, cols_, 0, buffer_size_ - 1);
                        uploadSinoBuffer(begin_wrt_geom, end_wrt_geom);
                    } else {
                        utils::projection2sino(
                            buffer_, sino_buffer_, rows_, cols_, 0, num_projections_ - 1 - begin_wrt_geom);
                        // we have gone around in the geometry
                        uploadSinoBuffer(begin_wrt_geom, num_projections_ - 1);

                        utils::projection2sino(
                            buffer_, sino_buffer_, rows_, cols_, num_projections_ - begin_wrt_geom, buffer_size_ - 1);
                        uploadSinoBuffer(0, end_wrt_geom);
                    }

                    ++update_count_;
                }

                reconstructPreview();
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
    // the lock is supposed to be always open if reconstruction mode == alternating
    std::lock_guard<std::mutex> guard(gpu_mutex_);
    return solver_->reconstructSlice(x, active_gpu_buffer_index_);
}

const std::vector<float>& Reconstructor::previewData() { 
    std::unique_lock<std::mutex> preview_lock(preview_mutex_);
    preview_cv_.wait(preview_lock);
    preview_buffer_.swap();
    return preview_buffer_.front(); 
}

int Reconstructor::previewSize() const { return preview_size_; }

int Reconstructor::bufferSize() const { return buffer_size_; }

void Reconstructor::processProjections(int begin, int end) {
#if defined(WITH_MONITOR)
    auto start = std::chrono::steady_clock::now();
#endif

    auto data = &buffer_[begin * pixels_];

    using namespace oneapi;
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, end - begin + 1),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                float* proj = &data[i * pixels_];

                utils::flatField(proj, pixels_, dark_avg_, reciprocal_);

                if (paganin_) paganin_->apply(proj, i % num_threads_);
                else
                    utils::negativeLog(proj, pixels_);

                filter_->apply(proj, tbb::this_task_arena::current_thread_index());

                // TODO: Add FDK scaler for cone beam
            }
        });
    });

#if defined(WITH_MONITOR)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Processing projection buffer took {} ms", duration / 1000);
#endif
}

void Reconstructor::uploadSinoBuffer(int begin, int end) {
    spdlog::info("Uploading sinogram buffer between {} and {} to GPU ...", begin, end);

    if (recon_mode_ == ReconstructMode::alternating) {
        active_gpu_buffer_index_ = 1 - active_gpu_buffer_index_;
        std::lock_guard<std::mutex> guard(gpu_mutex_);
        solver_->uploadProjections(active_gpu_buffer_index_, sino_buffer_, begin, end);
    } else {
        solver_->uploadProjections(active_gpu_buffer_index_, sino_buffer_, begin, end);
    }
}

void Reconstructor::reconstructPreview() {
    {
        std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
        {
            std::lock_guard<std::mutex> preview_lock(preview_mutex_);
            solver_->reconstructPreview(preview_buffer_.back(), active_gpu_buffer_index_);
            preview_cv_.notify_one();
        }
    }
    spdlog::info("Reconstructed low resolution preview from buffer {}.", active_gpu_buffer_index_);
}

const std::vector<RawDtype>& Reconstructor::darks() const { return all_darks_; }
const std::vector<RawDtype>& Reconstructor::flats() const { return all_flats_; }
const std::vector<float>& Reconstructor::buffer() const { return buffer_; }
const std::vector<float>& Reconstructor::sinoBuffer() const { return sino_buffer_; }

} // namespace slicerecon
