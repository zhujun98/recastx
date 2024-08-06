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
class DeviceTensor {
  public:

    using ValueType = T;
    using ShapeType = std::array<size_t, 3>;

  private:

    T* data_ = nullptr;
    ShapeType shape_;

  public:

    DeviceTensor();

    ~DeviceTensor();

    DeviceTensor(const DeviceTensor&) = delete;
    DeviceTensor& operator=(const DeviceTensor&) = delete;

    float* data() { return data_; }
    [[nodiscard]] const float* data() const { return data_; }

    void swap(DeviceTensor& other) noexcept;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return shape_; }
};

class TripleGpuTensorBuffer : public TripleBuffer<DeviceTensor<ProDtype, 3>> {

  public:

    using BufferType = DeviceTensor<ProDtype, 3>;
    using ValueType = typename BufferType::ValueType ;
    using ShapeType = typename BufferType::ShapeType;

  public:

    TripleGpuTensorBuffer();
    ~TripleGpuTensorBuffer() override;

    void resize(const ShapeType& shape);

    [[nodiscard]] const ShapeType& shape() const { return this->front_.shape(); }
};

using SinogramBuffer = TripleGpuTensorBuffer;
using VolumeBuffer = TripleGpuTensorBuffer;

} // recastx::recon

#endif // RECON_CUDA_BUFFER_H