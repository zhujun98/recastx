/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <functional>

#include <recon/cuda/utils.cuh>
#include "recon/cuda/buffer.cuh"

namespace recastx::recon {

template<typename T, size_t N>
DeviceTensor<T, N>::DeviceTensor() = default;

template<typename T, size_t N>
DeviceTensor<T, N>::~DeviceTensor() {
    if (data_ != nullptr) cudaFreeHost(data_);
}

template<typename T, size_t N>
void DeviceTensor<T, N>::swap(DeviceTensor& other) noexcept {
    std::swap(data_, other.data_);
    shape_.swap(other.shape_);
}

template<typename T, size_t N>
void DeviceTensor<T, N>::resize(const ShapeType& shape) {
    if (data_ != nullptr) cudaFreeHost(data_);
    size_t n = std::accumulate(std::begin(shape), std::end(shape), 1, std::multiplies<size_t>());
    checkCudaError(cudaMallocHost((void**)&data_, n * sizeof(ValueType)));
    shape_ = shape;
}

template class DeviceTensor<ProDtype, 3>;
template class DeviceTensor<ProDtype, 2>;


TripleGpuTensorBuffer::TripleGpuTensorBuffer() = default;

TripleGpuTensorBuffer::~TripleGpuTensorBuffer() = default;

void TripleGpuTensorBuffer::resize(const ShapeType& shape) {
    std::lock_guard lk(this->mtx_);
    this->back_.resize(shape);
    this->ready_.resize(shape);
    this->front_.resize(shape);
}

} // recastx::recon