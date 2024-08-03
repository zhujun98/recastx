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

class GpuTensor {
  public:

    using ValueType = float;
    using ShapeType = typename Tensor<float, 3>::ShapeType;

  private:

    float* data_ = nullptr;
    ShapeType shape_;

  public:

    GpuTensor();

    ~GpuTensor();

    float* data() { return data_; }
    [[nodiscard]] const float* data() const { return data_; }

    void swap(GpuTensor& other) noexcept;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return shape_; }
};

class TripleGpuTensorBuffer : public TripleBuffer<GpuTensor> {

  public:

    using BufferType = GpuTensor;
    using ValueType = typename BufferType::ValueType ;
    using ShapeType = typename BufferType::ShapeType;

  public:

    TripleGpuTensorBuffer() = default;
    ~TripleGpuTensorBuffer() override = default;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return this->front_.shape(); }
//
//    [[nodiscard]] size_t size() const { return this->front_.size(); }
};

} // recastx::recon

#endif // RECON_CUDA_BUFFER_H