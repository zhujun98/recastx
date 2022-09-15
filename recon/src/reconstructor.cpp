#include <chrono>
#include <complex>
#include <numeric>
using namespace std::chrono_literals;

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"


namespace tomcat::recon {

Reconstructor::Reconstructor(int rows, int cols, int num_threads)
     : rows_(rows),
       cols_(cols),
       pixels_(rows * cols),
       num_threads_(num_threads) {}

Reconstructor::~Reconstructor() = default;

void Reconstructor::initialize(int num_darks, 
                               int num_flats, 
                               int group_size,
                               int buffer_size,
                               int preview_size,
                               int slice_size) {
    num_darks_ = num_darks;
    num_flats_ = num_flats;
    group_size_ = group_size;
    preview_size_ = preview_size;
    slice_size_ = slice_size;

    all_darks_.resize(pixels_ * num_darks);
    all_flats_.resize(pixels_ * num_flats);
    dark_avg_.resize(pixels_);
    reciprocal_.resize(pixels_, 1.0f);

    buffer_size_ = buffer_size;
    buffer_.initialize(buffer_size_, group_size_, pixels_);
    sino_buffer_.initialize(group_size_ * pixels_);
    preview_buffer_.initialize(preview_size * preview_size * preview_size);

    initialized_ = true;
    spdlog::info("Reconstructor initialized:");
    spdlog::info("- Number of required dark images: {}", num_darks_);
    spdlog::info("- Number of required flat images: {}", num_flats_);
    spdlog::info("- Number of projection images per tomogram: {}", group_size_);
    spdlog::info("- Preview size: {}", preview_size_);
    spdlog::info("- Slice size: {}", slice_size_);
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

void Reconstructor::startProcessing() {
    processing_thread_ = std::thread([&] {
        oneapi::tbb::task_arena arena(num_threads_);
        while (true) {
            buffer_.fetch();
            spdlog::info("Processing projections ...");
            processProjections(arena);
        }
    });

    processing_thread_.detach();
}

void Reconstructor::startReconstructing() {
    gpu_upload_thread_ = std::thread([&] {
        while (true) {
            sino_buffer_.fetch();

            spdlog::info("Uploading sinograms to GPU ...");
            solver_->uploadSinograms(
                1 - gpu_buffer_index_, sino_buffer_.front(), 0, group_size_ - 1);

            {
                std::lock_guard<std::mutex> lck(gpu_mutex_);
                sino_uploaded_ = true;
                gpu_buffer_index_ = 1 - gpu_buffer_index_;
            }
            gpu_cv_.notify_one();
        }
    });

    gpu_recon_thread_ = std::thread([&] {

#if (VERBOSITY >= 1)
        const float data_size = group_size_ * pixels_ * sizeof(RawDtype) / (1024.f * 1024.f); // in MB
        auto start = std::chrono::steady_clock::now();
        size_t count = 0;
        float total_duration = 0.f;
#endif
 
        while (true) {
            {
                std::unique_lock<std::mutex> lck(gpu_mutex_);
                if (gpu_cv_.wait_for(lck, 20ms, [&] { return sino_uploaded_; })) {
                    solver_->reconstructPreview(preview_buffer_.back(), gpu_buffer_index_);
                } else {    
                    std::lock_guard lk(slice_mtx_);
                    for (auto slice_id : updated_slices_) {
                        solver_->reconstructSlice(slices_buffer_[slice_id], slices_[slice_id], gpu_buffer_index_);
                    }
                    updated_slices_.swap(reconstructed_slices_);
                    slice_cv_.notify_one();
                    continue;
                }

                std::lock_guard lk(slice_mtx_);
                for (const auto& [slice_id, orientation] : slices_) {
                    solver_->reconstructSlice(slices_buffer_[slice_id], orientation, gpu_buffer_index_);
                }
                updated_slices_.clear();
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
                buffer_.reset();

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
            buffer_.fill<RawDtype>(data, proj_idx / group_size_, proj_idx % group_size_);
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
            memcpy(&all_darks_[(received_darks_ - 1) * pixels_], data, sizeof(RawDtype) * pixels_);
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
            memcpy(&all_flats_[(received_flats_ - 1) * pixels_], data, sizeof(RawDtype) * pixels_);
            break;
        }
        default:
            break;
    }
}

void Reconstructor::setSlice(int slice_id, const Orientation& orientation) {
    std::lock_guard lk(slice_mtx_);
    if (slices_.find(slice_id) == slices_.end()) {
        std::vector<float> slice_buffer(slice_size_ * slice_size_);
        slices_buffer_[slice_id] = std::move(slice_buffer);

#if (VERBOSITY >= 3)
        spdlog::info("Slice {} added", slice_id);
#endif

    }

    slices_[slice_id] = orientation;
    updated_slices_.erase(slice_id);
    updated_slices_.insert(slice_id);

#if (VERBOSITY >= 3)
    spdlog::info("Slice {} updated", slice_id);
#endif

}

void Reconstructor::removeSlice(int slice_id) {
    std::lock_guard lk(slice_mtx_);
    slices_.erase(slice_id);
    slices_buffer_.erase(slice_id);

#if (VERBOSITY >= 3)
        spdlog::info("Slice {} removed", slice_id);
#endif

}

std::optional<VolumeDataPacket> Reconstructor::previewDataPacket() { 
    if(preview_buffer_.fetch(10));
        return VolumeDataPacket({preview_size_, preview_size_, preview_size_}, 
                                preview_buffer_.front());
    return {};
}

std::vector<SliceDataPacket> Reconstructor::sliceDataPackets() {
    std::vector<SliceDataPacket> ret;
    {
        std::lock_guard lk(slice_mtx_);
        for (const auto& [slice_id, data] : slices_buffer_) {
            ret.emplace_back(SliceDataPacket(slice_id, {slice_size_, slice_size_}, data));
        }
    }    
    return ret;
}

std::vector<SliceDataPacket> Reconstructor::updatedSliceDataPackets() {
    std::vector<SliceDataPacket> ret;
    {
        std::unique_lock<std::mutex> lck(slice_mtx_);
        slice_cv_.wait(lck, [&] { return !reconstructed_slices_.empty(); });
        for (auto slice_id : reconstructed_slices_) {
            ret.emplace_back(SliceDataPacket(
                slice_id, {slice_size_, slice_size_}, slices_buffer_[slice_id]));
        }
        reconstructed_slices_.clear();
    }
    return ret;
}

size_t Reconstructor::bufferSize() const { return group_size_; }

void Reconstructor::processProjections(oneapi::tbb::task_arena& arena) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    auto projs = buffer_.front().data();
    using namespace oneapi;
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, group_size_),
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
    arena.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, rows_),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                for (size_t j = 0; j < group_size_; ++j) {
                    for (size_t k = 0; k < cols_; ++k) {
                        sinos[i * group_size_ * cols_ + j * cols_ + k] = 
                            projs[j * cols_ * rows_ + i * cols_ + k];
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

const std::vector<RawDtype>& Reconstructor::darks() const { return all_darks_; }
const std::vector<RawDtype>& Reconstructor::flats() const { return all_flats_; }
const MemoryBuffer<float>& Reconstructor::buffer() const { return buffer_; }
const TripleBuffer<float>& Reconstructor::sinoBuffer() const { return sino_buffer_; }

} // tomcat::recon
