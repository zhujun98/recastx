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

class DeviceTensor {
  public:

    using ValueType = float;
    using ShapeType = typename Tensor<float, 3>::ShapeType;

  private:

    float* data_ = nullptr;
    ShapeType shape_;

  public:

    DeviceTensor();

    ~DeviceTensor();

    float* data() { return data_; }
    [[nodiscard]] const float* data() const { return data_; }

    void swap(DeviceTensor& other) noexcept;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return shape_; }
};

class TripleGpuTensorBuffer : public TripleBuffer<DeviceTensor> {

  public:

    using BufferType = DeviceTensor;
    using ValueType = typename DeviceTensor::ValueType ;
    using ShapeType = typename DeviceTensor::ShapeType;

  public:

    TripleGpuTensorBuffer();
    ~TripleGpuTensorBuffer() override;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return this->front_.shape(); }
};

using SinogramBuffer = TripleGpuTensorBuffer;

} // recastx::recon

#endif // RECON_CUDA_BUFFER_H