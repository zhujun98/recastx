/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>
#include <cmath>
#include <cstring>

#include "models/cube_model.hpp"

CubeModel::CubeModel(uint32_t x)
        : x_(x), data_(x_ * x_ * x_, 0.f) {
    genData();
}

std::string CubeModel::data() const {
    std::string data;
    data.resize(x_ * x_ * x_ * sizeof(DataType::value_type));
    std::memcpy(data.data(), data_.data(), data.size());
    return data;
}

void CubeModel::genData() {
    genDonut(x_ / 2, x_ / 2, x_ / 2, 3 * x_ / 8, x_/ 16, 0.2f);
    genDonut(x_ / 2, x_ / 2, x_ / 2, 3 * x_ / 8, x_/ 32, 0.5f);

    genCylinder(5 * x_ / 8, 5 * x_ / 8, x_ / 16, 11 * x_ / 16, x_ / 16, 0.1f);
    genCylinder(5 * x_ / 8, 5 * x_ / 8, x_ / 16, 11 * x_ / 16, x_ / 32, 1.f);

    genCylinder(3 * x_ / 8, 5 * x_ / 8, x_ / 8, 3 * x_ / 4, x_ / 16, 0.3f);
    genCylinder(3 * x_ / 8, 5 * x_ / 8, x_ / 8, 3 * x_ / 4, x_ / 32, 1.f);

    genCylinder(5 * x_ / 8, 3 * x_ / 8, 3 * x_ / 16, 13 * x_ / 16, x_ / 16, 0.3f);
    genCylinder(5 * x_ / 8, 3 * x_ / 8, 3 * x_ / 16, 13 * x_ / 16, x_ / 32, 1.f);

    genCylinder(3 * x_ / 8, 3 * x_ / 8, x_ / 4, 7 * x_ / 8, x_ / 16, 0.1f);
    genCylinder(3 * x_ / 8, 3 * x_ / 8, x_ / 4, 7 * x_ / 8, x_ / 32, 1.f);
}

void CubeModel::genDonut(uint32_t xc, uint32_t yc, uint32_t zc, uint32_t r1, uint32_t r2, DataType::value_type v) {
    assert(r1 > r2);

    size_t r22 = r2 * r2;
    size_t x2 = x_ * x_;
    size_t r = r1 + r2;
    for (size_t i = xc - r; i <= xc + r; ++i) {
        for (size_t j = yc - r; j <= yc + r; ++j) {
            for (size_t k = zc - r2; k <= zc + r2; ++k) {
                float r = std::sqrt((i - xc) * (i - xc) + (j - yc) * (j - yc));
                if ((r - r1) * (r - r1) + (k - zc) * (k - zc) <= r22) data_[i * x2 + j * x_ + k] = v;
            }
        }
    }
}

void CubeModel::genCylinder(uint32_t xc, uint32_t yc, uint32_t z0, uint32_t z1, uint32_t r, DataType::value_type v) {
    size_t x2 = x_ * x_;
    size_t r2 = r * r;
    for (size_t i = xc - r; i <= xc + r; ++i) {
        for (size_t j = yc - r; j <= yc + r; ++j) {
            for (size_t k = z0; k <= z1; ++k) {
                if ((i - xc) * (i - xc) + (j - yc) * (j - yc) <= r2) data_[i * x2 + j * x_ + k] = v;
            }
        }
    }
}