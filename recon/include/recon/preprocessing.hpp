#ifndef RECON_PREPROCESSING_H
#define RECON_PREPROCESSING_H

#include <vector>

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "tensor.hpp"

namespace recastx::recon {

inline void computeReciprocal(const RawImageGroup& darks,
                              const RawImageGroup& flats,
                              ProImageData& reciprocal,
                              ProImageData& dark_avg) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif
    darks.average<float>(dark_avg);
    auto flat_avg = flats.average<float>();
    for (size_t i = 0; i < reciprocal.shape()[0] * reciprocal.shape()[1]; ++i) {
        if (dark_avg[i] == flat_avg[i]) {
            reciprocal[i] = 1.0f;
        } else {
            reciprocal[i] = 1.0f / (flat_avg[i] - dark_avg[i]);
        }
    }
#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Computing reciprocal took {} ms", duration/1000);
#endif
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