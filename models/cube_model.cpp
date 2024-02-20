/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "models/cube_model.hpp"

CubeModel::CubeModel()
        : x_(512), y_(512), z_(512), data_(x_ * y_ * z_, 0.f) {
    for (size_t i = 128; i < 384; ++i) {
        for (size_t j = 128; j < 384; ++j) {
            for (size_t k = 128; k < 384; ++k) {
                data_[i * y_ * z_ + j * z_ + k] = 1.f;
            }
        }
    }
}

std::string CubeModel::data() const {
    std::string data;
    data.resize(x_ * y_ * z_ * sizeof(DataType::value_type));
    std::memcpy(data.data(), data_.data(), data.size());
    return data;
}