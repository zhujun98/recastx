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

#include "config.hpp"

namespace recastx {

inline size_t sliceIdFromTimestamp(size_t ts) {
    return ts % MAX_NUM_SLICES; 
}

inline size_t expandDataSize(size_t s, size_t chunk_size) {
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

template<typename T>
void computeHistogram(const std::vector<T>& data, double left, double right, size_t num_bins,
                      std::pair<std::vector<T>, std::vector<T>>& output) {
    auto& bin_centers = output.first;
    auto& bin_counts = output.second;

    bin_centers.resize(num_bins);
    bin_counts.resize(num_bins);

    if (left == right) {
        left -= 0.5;
        right += 0.5;
    }

    double norm = 1 / (right - left);
    size_t counted = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        auto v = static_cast<double>(data[i]);
        if (v >= left && v < right) {
            auto i_bin = static_cast<size_t>(static_cast<double>(num_bins) * (v - left) * norm);
            ++bin_counts[i_bin];
            ++counted;
        } else if (v == right) {
            ++bin_counts[num_bins - 1];
            ++counted;
        }
    }

    double bin_width = (right - left) / num_bins;
    bin_centers[0] = left + 0.5 * bin_width;
    for (size_t i = 1; i < bin_centers.size(); ++i) {
        bin_centers[i] += bin_centers[i - 1] + bin_width;
    }

    double scale = 1.f / (counted * bin_width);
    for (size_t i = 0; i < num_bins; ++i) {
        bin_counts[i] *= scale;
    }
}

} // namespace recastx

#endif // COMMON_UTILS_H