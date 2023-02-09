#ifndef SLICERECON_TENSOR_H
#define SLICERECON_TENSOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <unordered_set>

#include "tomcat/tomcat.hpp"

namespace tomcat::recon {

template<typename T, size_t N>
class Tensor {

public:

    using ShapeType = std::array<size_t, N>;

protected:

    ShapeType shape_;
    std::vector<T> data_;

public:

    using value_type = typename std::vector<T>::value_type;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    Tensor() = default;
    explicit Tensor(const ShapeType& shape);

    virtual ~Tensor() = default; 

    Tensor(const Tensor& other) = default;
    Tensor& operator=(const Tensor& other) = default;

    Tensor(Tensor&& other) = default;
    Tensor& operator=(Tensor&& other) = default;

    Tensor& operator=(std::initializer_list<T> ilist); 

    void resize(const ShapeType& shape);

    void resize(const ShapeType& shape, T value);

    template<typename R = T>
    void average(Tensor<R, N-1>& out) const;

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

    const std::array<size_t, N>& shape() { return shape_; }

    size_t size() const { return data_.size(); }
};

template<typename T, size_t N>
Tensor<T, N>::Tensor(const ShapeType& shape)
    : shape_(shape), 
      data_(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>())) {}

template<typename T, size_t N>
Tensor<T, N>& Tensor<T, N>::operator=(std::initializer_list<T> ilist) {
    data_ = ilist;
    return *this;
}

template<typename T, size_t N>
void Tensor<T, N>::resize(const ShapeType& shape) {
    data_.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>()));
    shape_ = shape;
}

template<typename T, size_t N>
void Tensor<T, N>::resize(const ShapeType& shape, T value) {
    data_.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>()), value);
}

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

using ImageData = Tensor<float, 2>;
using RawImageData = Tensor<RawDtype, 2>;

template<typename T>
class ImageGroup : public Tensor<T, 3> {

public:

    using typename Tensor<T, 3>::ShapeType;

private:

    size_t count_ = 0;

public:

    ImageGroup() = default;
    explicit ImageGroup(const ShapeType& shape);

    ~ImageGroup() override = default; 

    ImageGroup(const ImageGroup& other) = default;
    ImageGroup& operator=(const ImageGroup& other) = default;

    ImageGroup(ImageGroup&& other) = default;
    ImageGroup& operator=(ImageGroup&& other) = default;

    using Tensor<T, 3>::operator=;

    size_t push(const char* buffer);
    size_t push(const char* buffer, const std::array<size_t, 2>& shape);

    void reset() { count_ = 0; }

    bool full() const { return count_ >= this->shape_[0]; }

    bool empty() const { return count_ == 0; }

};

template<typename T>
ImageGroup<T>::ImageGroup(const ShapeType& shape) : Tensor<T, 3>(shape) {}

template<typename T>
size_t ImageGroup<T>::push(const char* buffer) {
    size_t idx = count_++ % this->shape_[0];
    size_t pixels = this->shape_[1] * this->shape_[2];
    std::memcpy(&this->data_[idx * pixels], buffer, pixels * sizeof(T));
    return count_;
}

template<typename T>
size_t ImageGroup<T>::push(const char* buffer, const std::array<size_t, 2>& shape) {
    if (shape[0] == this->shape_[1] && shape[1] == this->shape_[2]) {
        push(buffer);
    } else {
        push(buffer);
    }
    return count_;
}

using ProImageGroup = ImageGroup<ProDtype>;
using RawImageGroup = ImageGroup<RawDtype>;

} // tomcat::recon

#endif // SLICERECON_TENSOR_H