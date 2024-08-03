/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_BUFFER_H
#define RECON_CUDA_BUFFER_H

#include "recon/buffer.hpp"

namespace recastx::recon {

template<typename T, size_t N>
class GpuTensor {
  public:

    using ValueType = T;
    using ShapeType = typename Tensor<T, N>::ShapeType;

  private:

    std::vector<T> data_;
    ShapeType shape_;

  public:

    T* data() { return data_.data(); }
    [[nodiscard]] const T* data() const { return data_.data(); }

    void swap(GpuTensor& other) noexcept {
        data_.swap(other.data_);
        shape_.swap(other.shape_);
    }

    bool resize(const ShapeType& shape) {
        data_.resize(shape[0] * shape[1] * shape[2]);
        shape_ = shape;
        return true;
    }

    [[nodiscard]] ShapeType shape() const { return shape_; }
};

template<typename T, size_t N>
class TripleGpuTensorBuffer : public TripleBuffer<GpuTensor<T, N>> {

  public:

    using BufferType = GpuTensor<T, N>;
    using ValueType = typename BufferType::ValueType ;
    using ShapeType = typename BufferType::ShapeType;

  public:

    TripleGpuTensorBuffer() = default;
    ~TripleGpuTensorBuffer() override = default;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return this->front_.shape(); }

    [[nodiscard]] size_t size() const { return this->front_.size(); }
};

template<typename T, size_t N>
void TripleGpuTensorBuffer<T, N>::resize(const ShapeType& shape) {
    std::lock_guard(this->mtx_);
    this->back_.resize(shape);
    this->ready_.resize(shape);
    this->front_.resize(shape);
}

} // recastx::recon

#endif // RECON_CUDA_BUFFER_H