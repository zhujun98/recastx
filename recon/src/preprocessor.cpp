/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "common/scoped_timer.hpp"
#include "recon/preprocessor.hpp"
#include "recon/preprocessing.hpp"

namespace recastx::recon {

Preprocessor::Preprocessor(FilterFactory *ramp_filter_factory, uint32_t num_threads)
        : num_threads_(num_threads),
    arena_(oneapi::tbb::task_arena(num_threads_)),
    ramp_filter_factory_(ramp_filter_factory) {
}

void Preprocessor::init(RawBufferType& buffer, size_t col_count, size_t row_count,
          const ImageprocParams& imgproc_params,
          const std::optional<PaganinParams>& paganin_cfg) {
    initFilter(imgproc_params, buffer, col_count, row_count);
    initPaganin(paganin_cfg, buffer, col_count, row_count);
    minus_log_ = imgproc_params.minus_log;

    spdlog::info("[Init] - Ramp filter: {}", imgproc_params.ramp_filter.name);
    spdlog::info("[Init] - Number of image-processing threads: {}", num_threads_);
}

void Preprocessor::process(RawBufferType& raw_buffer,
                           ProDtype* sino_buffer,
                           const ProImageData& dark_avg,
                           const ProImageData& reciprocal,
                           int32_t offset) {
    auto& shape = raw_buffer.shape();
    auto [chunk_size, row_count, col_count] = shape;
    size_t num_pixels = row_count * col_count;

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Preprocessing projections");
#endif

    auto& projs = raw_buffer.front();

    using namespace oneapi;
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(chunk_size)),
                          [&](const tbb::blocked_range<int> &block) {
                              for (auto i = block.begin(); i != block.end(); ++i) {
                                  float* p = &projs[i * num_pixels];

                                  flatField(p, num_pixels, dark_avg, reciprocal);

                                  if (paganin_) {
                                      paganin_->apply(p, i % num_threads_);
                                  } else if (minus_log_) {
                                      negativeLog(p, num_pixels);
                                  }

                                  // FIXME: performance drops significantly if any dimension is not a produce of
                                  //        powers of small primes!!!
                                  ramp_filter_->apply(p, tbb::this_task_arena::current_thread_index());

                                  // TODO: Add FDK scaler for cone beam

                                  copyToSinogram(sino_buffer, projs, i, chunk_size, row_count, col_count, offset);
                              }
        });
    });
}

void Preprocessor::initPaganin(const std::optional<PaganinParams> &params,
                               RawBufferType &buffer,
                               size_t col_count,
                               size_t row_count) {
    if (params.has_value()) {
        auto &p = params.value();
        paganin_ = std::make_unique<Paganin>(
                p.pixel_size, p.lambda, p.delta, p.beta, p.distance, &buffer.front()[0], col_count, row_count);
    }
}

void Preprocessor::initFilter(const ImageprocParams &params,
                              RawBufferType &buffer,
                              size_t col_count,
                              size_t row_count) {
    ramp_filter_ = ramp_filter_factory_->create(
            params.ramp_filter.name, &buffer.front()[0], col_count, row_count, params.num_threads);
}

} // namespace recastx::recon