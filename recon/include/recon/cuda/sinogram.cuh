/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_SINOGRAM_LOADER_H
#define RECON_CUDA_SINOGRAM_LOADER_H

#include "astra/Float32ProjectionData3DGPU.h"

#include "recon/buffer.hpp"

namespace recastx::recon {

class Stream;

template<typename T, size_t N>
class GpuTensor {
  public:

    using ValueType = T;
    using ShapeType = std::array<size_t, N>;

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
};

template<typename T, size_t N>
class TripleGpuTensorBuffer : public TripleBuffer<Tensor<T, N>> {

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

class SinogramManager {

    using BufferType = TripleGpuTensorBuffer<float, 3>;

    BufferType buffer_;

    size_t start_;
    size_t group_size_;

    std::unique_ptr<Stream> stream_;

    void copyToDevice(astra::CFloat32ProjectionData3DGPU *proj,
                      const float *data, unsigned int y_min, unsigned int y_max);

  public:

    explicit SinogramManager(size_t group_size);

    ~SinogramManager();

    void load(astra::CFloat32ProjectionData3DGPU *dst);

    bool tryPrepareBuffer() {
        return buffer_.tryPrepare(100);
    }

    bool fetchBuffer() {
        return buffer_.fetch(100);
    }

    void reshapeBuffer(BufferType::ShapeType shape) {
        buffer_.resize(shape);
    }

    [[nodiscard]] ProDtype* buffer() { return buffer_.back().data(); }
};

} // recastx::recon

#endif // RECON_CUDA_SINOGRAM_LOADER_H