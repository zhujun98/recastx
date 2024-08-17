/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <iostream>
#include <limits>

#include <glm/glm.hpp>

#include "config.hpp"

namespace recastx {

inline size_t sliceIdFromTimestamp(size_t ts) {
    return ts % MAX_NUM_SLICES; 
}

inline size_t expandDataSizeForGpu(size_t s, size_t chunk_size) {
    return s % chunk_size == 0 ? s : (s / chunk_size + 1 ) * chunk_size;
}

template<typename T>
inline void arrayStat(const T& arr, std::array<size_t, 2> shape, const std::string& message = "") {
    double avg = 0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    for (size_t j = 0; j < shape[0]; ++j) {
        for (size_t k = 0; k < shape[1]; ++k) {
            auto v = arr[j * shape[1] + k];
            avg += v / static_cast<double>(shape[0] * shape[1]);
            if (v < min) min = v;
            if (v > max) max = v;
        }
    }

    if (message.empty()) {
        std::cout << "[Array statistics] ";
    } else {
        std::cout << "[" << message << "] ";
    }
    std::cout << "average: " << avg  << ", min: " << min << ", max: " << max << "\n";
}

} // namespace recastx

#endif // COMMON_UTILS_H