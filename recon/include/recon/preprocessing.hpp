/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PREPROCESSING_H
#define RECON_PREPROCESSING_H

#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "common/scoped_timer.hpp"
#include "tensor.hpp"

namespace recastx::recon {

namespace details {

template<typename T>
inline void copyToBuffer(Tensor<T, 2> &dst, const Tensor<T, 2> &src) {
    assert(dst.shape() == src.shape());
    memcpy(dst.data(), src.data(), src.size() * sizeof(T));
}

template<typename T>
inline void copyToBuffer(Tensor<T, 2> &dst, const Tensor<T, 2> &src, const std::array<size_t, 2> &downsampling) {
    const auto &src_shape = src.shape();
    const auto &dst_shape = dst.shape();
    if (dst_shape == src_shape) {
        assert(downsampling == (std::array < size_t, 2 > {1, 1}));
        copyToBuffer(dst, src);
        return;
    }

    auto [ds_r, ds_c] = downsampling;

    auto rows_ds = src_shape[0] / ds_r;
    auto cols_ds = src_shape[1] / ds_c;

    assert(dst_shape[0] >= rows_ds);
    assert(dst_shape[1] >= cols_ds);

    size_t padding_r = (dst_shape[0] - rows_ds) / 2;
    size_t padding_c = (dst_shape[1] - cols_ds) / 2;
    T* ptr_src = const_cast<T*>(src.data());
    T* ptr_dst = dst.data() + dst_shape[1] * padding_r;
    for (size_t i = 0; i < rows_ds; ++i) {
        for (size_t j = 0; j < cols_ds; ++j) {
            ptr_dst[j + padding_c] = ptr_src[ds_c * j];
        }
        ptr_src += ds_r * src_shape[1];
        ptr_dst += dst_shape[1];
    }
}

inline ProImageData computeReciprocal(const ProImageData &dark_avg, const ProImageData &flat_avg) {
    const auto &shape = dark_avg.shape();
    ProImageData reciprocal{shape};
    for (size_t i = 0; i < shape[0] * shape[1]; ++i) {
        if (dark_avg[i] == flat_avg[i]) {
            reciprocal[i] = 1.0f;
        } else {
            reciprocal[i] = 1.0f / (flat_avg[i] - dark_avg[i]);
        }
    }
    return reciprocal;
}

} // namespace details

inline void computeReciprocal(const std::vector<RawImageData> &darks,
                              const std::vector<RawImageData> &flats,
                              ProImageData& dark_avg,
                              ProImageData& reciprocal,
                              const std::array<size_t, 2>& downsampling) {
    assert(!darks.empty() || !flats.empty());

    Tensor<float, 2> flat_averaged;
    Tensor<float, 2> dark_averaged;
    if (darks.empty()) {
        flat_averaged = math::average<ProDtype>(flats);
        dark_averaged = flat_averaged * 0;
    } else if (flats.empty()) {
        dark_averaged = math::average<ProDtype>(darks);
        flat_averaged = dark_averaged + 1;
    } else {
        dark_averaged = math::average<ProDtype>(darks);
        flat_averaged = math::average<ProDtype>(flats);
    }

    auto reciprocal_orig = details::computeReciprocal(dark_averaged, flat_averaged);
    details::copyToBuffer(dark_avg, dark_averaged, downsampling);
    details::copyToBuffer(reciprocal, reciprocal_orig, downsampling);
}

inline void flatField(float *data,
                      size_t size,
                      const ProImageData &dark,
                      const ProImageData &reciprocal) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = (data[i] - dark[i]) * reciprocal[i];
    }
}

inline void negativeLog(float *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = data[i] <= 0.0f ? 0.0f : -std::log(data[i]);
    }
}

inline std::vector<float> defaultAngles(uint32_t n, float angle_range) {
    auto angles = std::vector<float>(n, 0.0f);
    std::iota(angles.begin(), angles.end(), 0.0f);
    std::transform(angles.begin(), angles.end(), angles.begin(),
                   [n, angle_range](auto x) { return x * M_PI * angle_range  / n; });
    return angles;
}

template<typename T1, typename T2>
inline void copyToSinogram(T1 *dst,
                           const T2 &src,
                           size_t chunk_idx,
                           size_t chunk_size,
                           size_t row_count,
                           size_t col_count,
                           int32_t offset) {
    // (chunk_idx, rows, cols) -> (rows, chunk_idx, cols).

    if (offset == 0) {
        for (size_t j = 0; j < row_count; ++j) {
            for (size_t k = 0; k < col_count; ++k) {
                dst[(row_count - 1 - j) * chunk_size * col_count + chunk_idx * col_count + k] =
                        src[chunk_idx * col_count * row_count + j * col_count + k];
            }
        }
    } else if (offset < 0) {
        for (size_t j = 0; j < row_count; ++j) {
            for (size_t k = col_count + offset; k < col_count; ++k) {
                dst[(row_count - 1 - j) * chunk_size * col_count + chunk_idx * col_count + k] = 0;
            }
            for (size_t k = 0; k < col_count + offset; ++k) {
                dst[(row_count - 1 - j) * chunk_size * col_count + chunk_idx * col_count + k] =
                        src[chunk_idx * col_count * row_count + j * col_count + k - offset];
            }
        }
    } else {
        for (size_t j = 0; j < row_count; ++j) {
            for (size_t k = 0; k < static_cast<size_t>(offset); ++k) {
                dst[(row_count - 1 - j) * chunk_size * col_count + chunk_idx * col_count + k] = 0;
            }
            for (size_t k = offset; k < col_count; ++k) {
                dst[(row_count - 1 - j) * chunk_size * col_count + chunk_idx * col_count + k] =
                        src[chunk_idx * col_count * row_count + j * col_count + k - offset];
            }
        }
    }
}

} // namespace recastx::recon

#endif // RECON_PREPROCESSING_H