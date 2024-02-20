/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cstring>

#include "models/cube_model.hpp"

CubeModel::CubeModel()
        : x_(512), y_(512), z_(512), data_(x_ * y_ * z_, 0.f) {
    genData();
}

std::string CubeModel::data() const {
    std::string data;
    data.resize(x_ * y_ * z_ * sizeof(DataType::value_type));
    std::memcpy(data.data(), data_.data(), data.size());
    return data;
}

void CubeModel::genSubData(uint32_t x0, uint32_t y0, uint32_t z0, size_t step_size) {
    for (size_t i = x0; i < x0 + step_size; ++i) {
        for (size_t j = y0; j < y0 + step_size; ++j) {
            for (size_t k = z0; k < z0 + step_size; ++k) {
                data_[i * y_ * z_ + j * z_ + k] = static_cast<float>(x0 + y0 + z0) / static_cast<float>(step_size);
            }
        }
    }
}

void CubeModel::genData() {
    const size_t step_size = 64;
    for (size_t i = 32; i < x_ - step_size; i += 2 * step_size) {
        for (size_t j = 32; j < y_ - step_size; j += 2 * step_size) {
            for (size_t k = 32; k < z_ - step_size; k += 2 * step_size) {
                genSubData(i, j, k, step_size);
            }
        }
    }
}