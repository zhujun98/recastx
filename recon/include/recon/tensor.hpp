#ifndef RECON_TENSOR_H
#define RECON_TENSOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <numeric>
#include <unordered_set>

#include "common/config.hpp"


namespace tomcat::recon {

template<typename T, size_t N>
class Tensor {

public:

    using ShapeType = std::array<size_t, N>;
    using ValueType = T;

protected:

    ShapeType shape_;
    std::vector<T> data_;

public:

    using value_type = typename std::vector<T>::value_type;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    Tensor() {
        shape_.fill(0);
    };

    explicit Tensor(const ShapeType& shape)
        : shape_(shape), 
          data_(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>())) {}
;

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

    virtual Tensor& operator=(std::initializer_list<T> ilist) {
        std::copy(ilist.begin(), ilist.end(), data_.begin());
        return *this;
    } 

    void swap(Tensor& other) noexcept {
        data_.swap(other.data_);
        shape_.swap(other.shape_);
    }

    void reshape(const ShapeType& shape) {
        data_.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>()));
        shape_ = shape;
    }

    void reshape(const ShapeType& shape, T value) {
        data_.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>()), value);
    }

    // FIXME: specialize for N == 1
    template<typename R = T>
    void average(Tensor<R, N-1>& out) const;

    // FIXME: specialize for N == 1
    template<typename R = T>
    Tensor<R, N-1> average() const;

    T& operator[](size_t pos) { return data_[pos]; }

    const T& operator[](size_t pos) const { return data_[pos]; }

    T* data() { return data_.data(); }

    const T* data() const { return data_.data(); }

    iterator begin() { return data_.begin(); }
    const_iterator begin() const { return data_.begin(); }

    iterator end() { return data_.end(); }
    const_iterator end() const { return data_.end(); }

    const ShapeType& shape() { return shape_; }

    size_t size() const { return data_.size(); }
};

template<typename T, size_t N>
template<typename R>
void Tensor<T, N>::average(Tensor<R, N-1>& out) const {
    size_t size = std::accumulate(shape_.begin() + 1, shape_.end(), 1, std::multiplies<>());
    for (size_t j = 0; j < size; ++j) out[j] = static_cast<R>(data_[j]);
    
    for (size_t i = 1; i < shape_[0]; ++i) {
        for (size_t j = 0, s = i * size ; j < size; ++j) {
            out[j] += static_cast<R>(data_[s + j]);
        }
    }

    for (size_t j = 0; j < size; ++j) out[j] /= shape_[0];
}

template<typename T, size_t N>
template<typename R>
Tensor<R, N-1> Tensor<T, N>::average() const {
    std::array<size_t, N-1> shape;
    std::copy(shape_.begin() + 1, shape_.end(), shape.begin());
    Tensor<R, N-1> out(shape);
    average(out);
    return out;
}

using ProImageData = Tensor<ProDtype, 2>;
using RawImageData = Tensor<RawDtype, 2>;

namespace details {

template<typename T>
inline void copyBuffer(T* dst, const char* src, size_t n) {
    std::memcpy(dst, src, n * sizeof(T));
}

template<typename T>
inline void copyBuffer(T* dst, const char* src, 
                       const std::array<size_t, 2>& dst_shape, 
                       const std::array<size_t, 2>& src_shape,
                       const std::array<size_t, 2>& downsampling) {
    for (size_t size = sizeof(T), 
             rstep = downsampling[0] * size * src_shape[1], 
             cstep = downsampling[1] * size, i = 0; i < dst_shape[0]; ++i) {
        char* ptr = const_cast<char*>(src) + i * rstep;
        for (size_t j = 0; j < dst_shape[1]; ++j) {
            memcpy(dst++, ptr, size);
            ptr += cstep;
        }
    }
}

} // details

template<typename T>
class ImageGroup : public Tensor<T, 3> {

public:

    using typename Tensor<T, 3>::ShapeType;
    using typename Tensor<T, 3>::ValueType;

private:

    size_t count_ = 0;

public:

    ImageGroup() = default;
    explicit ImageGroup(const ShapeType& shape) : Tensor<T, 3>(shape) {};

    ~ImageGroup() override = default; 

    ImageGroup(const ImageGroup& other) = default;
    ImageGroup& operator=(const ImageGroup& other) = default;

    ImageGroup(ImageGroup&& other)
             : Tensor<T, 3>(std::move(other)), count_(other.count_) {
        other.count_ = 0;
    }

    ImageGroup& operator=(ImageGroup&& other) {
        Tensor<T, 3>::operator=(std::move(other));
        count_ = other.count_;
        other.count_ = 0;
        return *this;
    }

    ImageGroup& operator=(std::initializer_list<T> ilist) override {
        Tensor<T, 3>::operator=(ilist);
        count_ = this->shape_[0];
        return *this;
    }

    size_t push(const char* buffer);
    size_t push(const char* buffer, 
                const std::array<size_t, 2>& shape, 
                const std::array<size_t, 2>& downsampling);

    void reset() { count_ = 0; }

    bool full() const { return count_ >= this->shape_[0]; }

    bool empty() const { return count_ == 0; }

};

template<typename T>
size_t ImageGroup<T>::push(const char* buffer) {
    const size_t idx = count_++ % this->shape_[0];
    const size_t pixels = this->shape_[1] * this->shape_[2];
    details::copyBuffer(&this->data_[idx * pixels], buffer, pixels);
    return count_;
}

template<typename T>
size_t ImageGroup<T>::push(const char* buffer, 
                           const std::array<size_t, 2>& shape, 
                           const std::array<size_t, 2>& downsampling) {
    size_t h = this->shape_[1];
    size_t w = this->shape_[2];
    if (downsampling[0] == 1 && downsampling[1] == 1) {
        assert(h == shape[0] && w == shape[1]);
        return push(buffer);
    }
    
    size_t idx = count_++ % this->shape_[0];
    size_t pixels = w * h;
    details::copyBuffer(&this->data_[idx * pixels], buffer, {h, w}, shape, downsampling);
    return count_;
}

using ProImageGroup = ImageGroup<ProDtype>;
using RawImageGroup = ImageGroup<RawDtype>;

} // tomcat::recon

#endif // RECON_TENSOR_H