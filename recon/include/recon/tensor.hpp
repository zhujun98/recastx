/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_TENSOR_H
#define RECON_TENSOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <unordered_set>

#include <fmt/core.h>

#include "common/config.hpp"


namespace recastx::recon {

template<typename T, size_t N>
class Tensor {

public:

    using ShapeType = std::array<size_t, N>;
    using ContainerType = std::vector<T>;
    using ValueType = T;

    using value_type = typename ContainerType::value_type;
    using iterator = typename ContainerType::iterator;
    using const_iterator = typename ContainerType::const_iterator;

private:

    static size_t size(const ShapeType& shape) {
        return std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
    }

protected:

    ShapeType shape_;
    ContainerType data_;

public:

    Tensor() { shape_.fill(0); };

    explicit Tensor(const ShapeType& shape) : shape_(shape), data_(size(shape)) {}

    Tensor(const ShapeType& shape, const std::vector<T>& data) : shape_(shape), data_(data) {
        size_t s = size(shape_);
        if (s != data_.size()) {
            throw std::runtime_error(fmt::format(
                "Tensor shape does not match size of the initializer list: {} and {}", s, data_.size()));
        }
    }

    Tensor(ShapeType&& shape, std::vector<T>&& data) : shape_(std::move(shape)), data_(std::move(data)) {
        size_t s = size(shape_);
        if (s != data_.size()) {
            throw std::runtime_error(fmt::format(
                "Tensor shape does not match size of the initializer list: {} and {}", s, data_.size()));
        }
    }

    virtual ~Tensor() = default; 

    Tensor(const Tensor& other) = default;
    Tensor& operator=(const Tensor& other) = default;

    Tensor(Tensor&& other) noexcept
            : shape_(other.shape_), data_(std::move(other.data_)) {
        other.shape_.fill(0);
    }

    Tensor& operator=(Tensor&& other) noexcept {
        shape_ = other.shape_;
        data_.swap(other.data_);
        other.shape_.fill(0);
        other.data_.clear();
        other.data_.shrink_to_fit();
        return *this;
    }

    void swap(Tensor& other) noexcept {
        data_.swap(other.data_);
        shape_.swap(other.shape_);
    }

    void resize(const ShapeType& shape) {
        data_.resize(size(shape));
        shape_ = shape;
    }

    void resize(const ShapeType& shape, T value) {
        data_.resize(size(shape), value);
    }

    T& operator[](size_t pos) { return data_[pos]; }

    const T& operator[](size_t pos) const { return data_[pos]; }

    T* data() { return data_.data(); }

    const T* data() const { return data_.data(); }

    iterator begin() { return data_.begin(); }
    const_iterator begin() const { return data_.begin(); }

    iterator end() { return data_.end(); }
    const_iterator end() const { return data_.end(); }

    Tensor<T, N>& operator+=(const Tensor<T, N>& rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] += rhs.data_[i];
        }
        return *this;
    }

    Tensor<T, N>& operator+=(T rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] += rhs;
        }
        return *this;
    }

    Tensor<T, N>& operator-=(const Tensor<T, N>& rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] -= rhs.data_[i];
        }
        return *this;
    }

    Tensor<T, N>& operator-=(T rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] -= rhs;
        }
        return *this;
    }

    Tensor<T, N>& operator*=(T rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] *= rhs;
        }
        return *this;
    }

    Tensor<T, N>& operator/=(T rhs) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] /= rhs;
        }
        return *this;
    }

    friend Tensor<T, N> operator+(Tensor<T, N> lhs, const Tensor<T, N>& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend Tensor<T, N> operator+(Tensor<T, N> lhs, T rhs) {
        lhs += rhs;
        return lhs;
    }

    friend Tensor<T, N> operator-(Tensor<T, N> lhs, const Tensor<T, N>& rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend Tensor<T, N> operator-(Tensor<T, N> lhs, T rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend Tensor<T, N> operator*(Tensor<T, N> lhs, T rhs) {
        lhs *= rhs;
        return lhs;
    }

    friend Tensor<T, N> operator/(Tensor<T, N> lhs, T rhs) {
        lhs /= rhs;
        return lhs;
    }

    const ShapeType& shape() const { return shape_; }

    size_t size() const { return data_.size(); }
};

using ProImageData = Tensor<ProDtype, 2>;
using RawImageData = Tensor<RawDtype, 2>;

namespace math {

namespace details {
    template<typename R, typename T, size_t N>
    inline void average(const std::vector<Tensor<T, N>>& src, Tensor<R, N>& dst) {
        const auto& shape = dst.shape();
        size_t size = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
        
        for (const auto& v : src) {
            for (size_t i = 0; i < size; ++i) {
                dst[i] += static_cast<R>(v[i]);
            }
        }

        dst /= static_cast<R>(src.size());
    }
}

// FIXME: specialize for N == 1
template<typename R, typename T, size_t N>
inline void average(const std::vector<Tensor<T, N>>& src, Tensor<R, N>& dst) {
    assert(!src.empty());
    assert(src[0].shape() == dst.shape());
    details::average(src, dst);
}

template<typename R = void, typename T, size_t N>
inline auto average(const std::vector<Tensor<T, N>>& src) {
    assert(!src.empty());
    using ReturnType = std::conditional_t<std::is_same<R, void>::value, double, R>;
    Tensor<ReturnType, N> dst(src[0].shape());
    details::average(src, dst);
    return dst;
} 

} // namespace math

namespace details {

template<typename T>
inline void copyBuffer(T* dst, const char* src, size_t count) {
    std::memcpy(dst, src, count * sizeof(T));
}

template<typename T>
inline void copyBuffer(T* dst,
                       const std::array<size_t, 2>& dst_shape,
                       const char* src,
                       const std::array<size_t, 2>& src_shape) {
    size_t ds_r = src_shape[0] / dst_shape[0];
    size_t ds_c = src_shape[1] / dst_shape[1];
    for (size_t size = sizeof(T), 
             rstep = ds_r * size * src_shape[1], 
             cstep = ds_c * size, i = 0; i < dst_shape[0]; ++i) {
        char* ptr = const_cast<char*>(src) + i * rstep;
        for (size_t j = 0; j < dst_shape[1]; ++j) {
            memcpy(dst++, ptr, size);
            ptr += cstep;
        }
    }
}

} // details

} // namespace recastx::recon

#endif // RECON_TENSOR_H