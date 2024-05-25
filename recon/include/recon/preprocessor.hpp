/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PREPROCESSOR_H
#define RECON_PREPROCESSOR_H

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include <spdlog/spdlog.h>
#include <oneapi/tbb.h>

#include "common/config.hpp"
#include "buffer.hpp"
#include "filter_interface.hpp"
#include "phase.hpp"

namespace recastx::recon {

class Preprocessor {

public:

    using RawBufferType = MemoryBuffer<ProDtype, 3>;
    using SinoBufferType = TripleTensorBuffer<ProDtype, 3>;

private:

    uint32_t num_threads_ = 1;
    oneapi::tbb::task_arena arena_;

    std::unique_ptr<Paganin> paganin_;

    FilterFactory *ramp_filter_factory_;
    std::unique_ptr<Filter> ramp_filter_;

    bool minus_log_ = true;

    void initPaganin(const std::optional<PaganinParams> &params,
                     RawBufferType &buffer, size_t col_count,
                     size_t row_count);

    void initFilter(const ImageprocParams &params,
                    RawBufferType &buffer,
                    size_t col_count,
                    size_t row_count);

public:

    explicit Preprocessor(FilterFactory *ramp_filter_factory, uint32_t num_threads);

    void init(RawBufferType &buffer, size_t col_count, size_t row_count,
              const ImageprocParams &imgproc_params,
              const std::optional<PaganinParams> &paganin_cfg);

    void process(RawBufferType &raw_buffer,
                 SinoBufferType &sino_buffer,
                 const ProImageData &dark_avg,
                 const ProImageData &reciprocal,
                 int32_t offset);

};

} // namespace recastx::recon

#endif // RECON_PREPROCESSOR_H