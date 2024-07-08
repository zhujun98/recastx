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
#include <random>

#include "models/cube_model.hpp"

CubeModel::CubeModel(uint32_t x)
        : x_(x), y_(x), z_(x), data_(x_ * y_ * z_, 0.f) {
}

CubeModel::CubeModel(uint32_t x, uint32_t y, uint32_t z)
        : x_(x), y_(y), z_(z), data_(x_ * y_ * z_, 0.f){
}

std::string CubeModel::data() const {
    std::string data;
    data.resize(x_ * x_ * x_ * sizeof(DataType::value_type));
    std::memcpy(data.data(), data_.data(), data.size());
    return data;
}

void CubeModel::genSpheres(size_t n) {
    uint32_t width = std::min(std::min(x_, y_), z_);
    uint32_t r_min = 2 * width / n;
    uint32_t r_max = 2 * r_min;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist_x(r_max, x_ - r_max - 1);
    std::uniform_int_distribution<uint32_t> dist_y(r_max, y_ - r_max - 1);
    std::uniform_int_distribution<uint32_t> dist_z(r_max, z_ - r_max - 1);
    std::uniform_int_distribution<uint32_t> dist_r(r_min, r_max);
    std::uniform_real_distribution<float> dist_density(0.1f, MAX_DENSITY);

    for (size_t i = 0; i < n; ++i) {
        genSphere(dist_x(mt), dist_y(mt), dist_z(mt), dist_r(mt), dist_density(mt));
    }
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

void CubeModel::genSphere(uint32_t xc, uint32_t yc, uint32_t zc, uint32_t r, DataType::value_type  v) {
    size_t r2 = r * r;
    for (size_t i = xc - r; i <= xc + r; ++i) {
        for (size_t j = yc - r; j <= yc + r; ++j) {
            for (size_t k = zc - r; k <= zc + r; ++k) {
                size_t dist2 = (i - xc) * (i - xc) + (j - yc) * (j - yc) + (k - zc) * (k - zc);
                size_t idx = i * y_ * z_ + j * z_ + k;
                if (dist2 <= r2 / 4) {
                    data_[idx] = MAX_DENSITY;
                } else if (dist2 <= r2) {
                    data_[idx] = std::min(MAX_DENSITY, data_[idx] + v);
                }
            }
        }
    }
}
