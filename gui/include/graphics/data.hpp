/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUME_DATA_H
#define GUI_VOLUME_DATA_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "common/utils.hpp"

namespace recastx::gui {

template<typename T>
class Data2D {

  public:

    using ValueType = T;

  private:

    uint32_t x_;
    uint32_t y_;
    std::vector<ValueType> data_;

    std::optional<std::array<ValueType, 2>> min_max_vals_;
    std::pair<std::vector<T>, std::vector<T>> histogram_;

  public:

    Data2D() : x_(0), y_(0) {}

    void setData(const std::string& data, uint32_t x, uint32_t y) {
        if (x != x_ || y != y_) {
            x_ = x;
            y_ = y;
            data_.resize(x * y);
        }

        assert(data.size() == x * y * sizeof(ValueType));
        std::memcpy(data_.data(), data.data(), data.size());

        min_max_vals_.reset();
        histogram_.first.clear();
        histogram_.second.clear();
    }

    [[nodiscard]] const std::optional<std::array<T, 2>>& minMaxVals() {
        if (!min_max_vals_ && !data_.empty()) {
            auto [v_min, v_max] = std::minmax_element(data_.begin(), data_.end());
            min_max_vals_ = {*v_min, *v_max};
        }
        return min_max_vals_;
    }

    const std::pair<std::vector<T>, std::vector<T>>& histogram() {
        if (histogram_.first.empty() && !data_.empty()) {
            auto [v_min, v_max] = minMaxVals().value();
            computeHistogram(data_, v_min, v_max, 120, histogram_);
        }
        return histogram_;
    }

    [[nodiscard]] bool empty() const { return data_.empty(); }

    const T* data() const { return data_.data(); }
    [[nodiscard]] uint32_t x() const { return x_; }
    [[nodiscard]] uint32_t y() const { return y_; }
    [[nodiscard]] uint32_t size() const { return data_.size(); }
};


template<typename T>
class Data3D {

  public:

    using ValueType = T;

  private:

    uint32_t x_;
    uint32_t y_;
    uint32_t z_;
    std::vector<ValueType> data_;

    std::optional<std::array<ValueType, 2>> min_max_vals_;
    std::pair<std::vector<T>, std::vector<T>> histogram_;

  public:

    Data3D() : x_(0), y_(0), z_(0) {}

    bool resize(uint32_t x, uint32_t y, uint32_t z) {
        if (x != x_ || y != y_ || z != z_) {
            x_ = x;
            y_ = y;
            z_ = z;
            data_.resize(x_ * y_ * z_, 0);
            return true;
        }
        return false;
    }

    void setData(const std::string& data, uint32_t x, uint32_t y, uint32_t z) {
        resize(x, y, z);
        assert(data.size() == x * y * z * sizeof(ValueType));
        std::memcpy(data_.data(), data.data(), data.size());
        min_max_vals_.reset();

        histogram_.first.clear();
        histogram_.second.clear();
    }

    bool setShard(const std::string& data, uint32_t pos) {
        assert(data.size() == x_ * y_ * sizeof(ValueType));
        assert(pos * sizeof(ValueType) + data.size() <= data_.size() * sizeof(ValueType));
        std::memcpy(data_.data() + pos, data.data(), data.size());

        if (pos == 0) {
            min_max_vals_.reset();
            histogram_.first.clear();
            histogram_.second.clear();
        }

        return pos * sizeof(ValueType) + data.size() == data_.size() * sizeof(ValueType);
    }

    [[nodiscard]] const std::optional<std::array<float, 2>>& minMaxVals() {
        if (!min_max_vals_ && !data_.empty()) {
            auto [v_min, v_max] = std::minmax_element(data_.begin(), data_.end());
            min_max_vals_ = {*v_min, *v_max};
        }
        return min_max_vals_;
    }

    const std::pair<std::vector<T>, std::vector<T>>& histogram() {
        if (histogram_.first.empty() && !data_.empty()) {
            auto [v_min, v_max] = minMaxVals().value();
            computeHistogram(data_, v_min, v_max, 120, histogram_);
        }
        return histogram_;
    }

    [[nodiscard]] bool empty() const { return data_.empty(); }

    const T* data() const { return data_.data(); }
    [[nodiscard]] uint32_t x() const { return x_; }
    [[nodiscard]] uint32_t y() const { return y_; }
    [[nodiscard]] uint32_t z() const { return z_; }
    [[nodiscard]] uint32_t size() const { return data_.size(); }

    void swap(Data3D<T>& other) noexcept {
        data_.swap(other.data_);
        std::swap(x_, other.x_);
        std::swap(y_, other.y_);
        std::swap(z_, other.z_);
        min_max_vals_.swap(other.min_max_vals_);
        histogram_.swap(other.histogram_);
    }

    void clear() {
        x_ = 0;
        y_ = 0;
        z_ = 0;
        data_.clear();
    }
};

}

#endif //GUI_VOLUME_DATA_H
