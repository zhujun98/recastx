/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PROJECTION_H
#define RECON_PROJECTION_H

#include <type_traits>

#include "common/config.hpp"
#include "recon/buffer.hpp"

namespace recastx::recon {

template<typename T = RawDtype>
struct Projection {
    ProjectionType type;
    size_t index;
    Tensor<T, 2> data;

    Projection() = default;

    Projection(ProjectionType type, size_t index, size_t col_count, size_t row_count, std::vector<T> data_)
        : type(type), index(index), data(std::array<size_t, 2> {row_count, col_count}, std::move(data_)) {
    }

    Projection(ProjectionType type, size_t index, size_t col_count, size_t row_count, const void* data_, size_t size)
            : type(type), index(index), data(std::array<size_t, 2> {row_count, col_count}) {
        std::memcpy(data.data(), data_, size);
    }
};

} // namespace recastx::recon

#endif // RECON_PROJECTION_H