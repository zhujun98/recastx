/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PREPROCESSING_H
#define RECON_PREPROCESSING_H

#include <cassert>
#include <vector>

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "common/scoped_timer.hpp"
#include "tensor.hpp"

namespace recastx::recon {

inline void downsample(const ProImageData& src, ProImageData& dst) {
    auto& src_shape = src.shape();
    auto& dst_shape = dst.shape();
    size_t ds_row = src_shape[0] / dst_shape[0];
    size_t ds_col = src_shape[1] / dst_shape[1];
    assert (ds_row >= 1);
    assert (ds_col >= 1);
    for (size_t i = 0; i < dst_shape[0]; ++i) {
        for (size_t j = 0; j < dst_shape[1]; ++j) {
            dst[i * dst_shape[1] + j] = src[i * src_shape[1] * ds_row + ds_col * j];
        }
    }
}

inline std::pair<ProImageData, ProImageData> computeReciprocal(
        const std::vector<RawImageData>& darks, const std::vector<RawImageData>& flats) {

    auto dark_avg = math::average<ProDtype>(darks);
    auto flat_avg = math::average<ProDtype>(flats);

    const auto& shape = dark_avg.shape();
    ProImageData reciprocal {shape};
    for (size_t i = 0; i < shape[0] * shape[1]; ++i) {
        if (dark_avg[i] == flat_avg[i]) {
            reciprocal[i] = 1.0f;
        } else {
            reciprocal[i] = 1.0f / (flat_avg[i] - dark_avg[i]);
        }
    }

    return {dark_avg, reciprocal};
}

inline void flatField(float* data, 
                      size_t size, 
                      const ProImageData& dark, 
                      const ProImageData& reciprocal) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = (data[i] - dark[i]) * reciprocal[i];
    } 
}

inline void negativeLog(float* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = data[i] <= 0.0f ? 0.0f : -std::log(data[i]);
    }
}

inline std::vector<float> defaultAngles(int n) {
    auto angles = std::vector<float>(n, 0.0f);
    std::iota(angles.begin(), angles.end(), 0.0f);
    std::transform(angles.begin(), angles.end(), angles.begin(),
                   [n](auto x) { return (x * M_PI) / n; });
    return angles;
}

} // namespace recastx::recon

#endif // RECON_PREPROCESSING_H