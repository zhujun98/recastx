/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "spdlog/spdlog.h"

#include "recon/cuda/memory.cuh"
#include "recon/cuda/utils.cuh"
#include "recon/cuda/stream.cuh"

namespace recastx::recon {

AstraMemHandleBase::AstraMemHandleBase() = default;

AstraMemHandleBase::~AstraMemHandleBase() = default;

AstraMemHandleBase::AstraMemHandleBase(AstraMemHandleBase&& other) noexcept : handle_(std::move(other.handle_)) {
}

AstraMemHandleBase& AstraMemHandleBase::operator=(AstraMemHandleBase&& other) noexcept {
    handle_ = std::move(other.handle_);
    return *this;
}

unsigned int AstraMemHandleBase::x() const { return handle_.d->nx; }
unsigned int AstraMemHandleBase::y() const { return handle_.d->ny; }
unsigned int AstraMemHandleBase::z() const { return handle_.d->nz; }
unsigned int AstraMemHandleBase::size() const { return handle_.d->nx * handle_.d->ny * handle_.d->nz;; }


AstraMemHandle::AstraMemHandle(unsigned int x, unsigned int y, unsigned int z) : AstraMemHandleBase() {
    handle_.d = boost::shared_ptr<astraCUDA3d::SMemHandle3D_internal>(new astraCUDA3d::SMemHandle3D_internal);
    handle_.d->arr = nullptr;

    if (!checkCudaError(cudaMalloc3D(&handle_.d->ptr, make_cudaExtent(sizeof(float) * x, y, z)))) {
        throw std::runtime_error("Failed to allocate a 3D logical array");
    }

    handle_.d->nx = x;
    handle_.d->ny = y;
    handle_.d->nz = z;

    auto [gpu_memory_available, gpu_memory_total] = queryGPUMemory();
    spdlog::debug("Allocated {} x {} x {} on GPU. ({} / {})", x, y, z, gpu_memory_available, gpu_memory_total);
}

AstraMemHandle::~AstraMemHandle() {
    if (handle_.d != nullptr) checkCudaError(cudaFree(handle_.d->ptr.ptr));
}

AstraMemHandle::AstraMemHandle(AstraMemHandle&& other) noexcept = default;

AstraMemHandle& AstraMemHandle::operator=(AstraMemHandle&& other) noexcept = default;

AstraMemHandleArray::AstraMemHandleArray(unsigned int x, unsigned int y, unsigned int z) : AstraMemHandleBase(){
    handle_.d = boost::shared_ptr<astraCUDA3d::SMemHandle3D_internal>(new astraCUDA3d::SMemHandle3D_internal);
    handle_.d->ptr = make_cudaPitchedPtr(nullptr, 0, 0, 0);

    auto channel_desc = cudaCreateChannelDesc<float>();
    auto extent = make_cudaExtent(x, y, z);
    if (!checkCudaError(cudaMalloc3DArray(&handle_.d->arr, &channel_desc, extent))) {
        throw std::runtime_error("Failed to allocate a 3D array");
    }

    handle_.d->nx = x;
    handle_.d->ny = y;
    handle_.d->nz = z;

    auto [gpu_memory_available, gpu_memory_total] = queryGPUMemory();
    spdlog::debug("Allocated {} x {} x {} on GPU. ({} / {})", x, y, z, gpu_memory_available, gpu_memory_total);
}

AstraMemHandleArray::~AstraMemHandleArray() {
    if (handle_.d != nullptr) checkCudaError(cudaFreeArray(handle_.d->arr));
}

AstraMemHandleArray::AstraMemHandleArray(AstraMemHandleArray&& other) noexcept = default;

AstraMemHandleArray& AstraMemHandleArray::operator=(AstraMemHandleArray&& other) noexcept = default;

} // recastx::recon