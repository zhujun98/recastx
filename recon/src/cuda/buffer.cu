/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <recon/cuda/utils.cuh>
#include "recon/cuda/buffer.cuh"

namespace recastx::recon {

DeviceTensor::DeviceTensor() = default;

DeviceTensor::~DeviceTensor() {
    if (data_ != nullptr) cudaFreeHost(data_);
}

void DeviceTensor::swap(DeviceTensor& other) noexcept {
    std::swap(data_, other.data_);
    shape_.swap(other.shape_);
}

void DeviceTensor::resize(const ShapeType& shape) {
    if (data_ != nullptr) cudaFreeHost(data_);
    checkCudaError(cudaMallocHost((void**)&data_, shape[0] * shape[1] * shape[2] * sizeof(ValueType)));
    shape_ = shape;
}

void TripleGpuTensorBuffer::resize(const ShapeType& shape) {
    std::lock_guard(this->mtx_);
    this->back_.resize(shape);
    this->ready_.resize(shape);
    this->front_.resize(shape);
}

} // recastx::recon