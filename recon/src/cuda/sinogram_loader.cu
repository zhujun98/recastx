/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "spdlog/spdlog.h"

#include "recon/cuda/sinogram_loader.cuh"
#include "recon/cuda/utils.cuh"
#include "recon/cuda/stream.cuh"

namespace recastx::recon {

SinogramLoader::SinogramLoader(size_t group_size)
    : start_{0}, group_size_{group_size}, stream_(new Stream) {
}

SinogramLoader::~SinogramLoader() = default;

void SinogramLoader::load(astra::CFloat32ProjectionData3DGPU *dst, const float *src, size_t count) {
    size_t start = start_;
    size_t end = (start + count - 1) % group_size_;

    if (end > start) {
        copyToDevice(dst, src, start, end);
        spdlog::debug("Uploaded sinograms {} - {}", start, end);
    } else {
        copyToDevice(dst, src, start, group_size_ - 1);
        spdlog::debug("Uploaded sinograms {} - {}", start, group_size_ - 1);
        copyToDevice(dst, src, 0, end);
        spdlog::debug("Uploaded sinograms {} - {}", 0, end);
    }

    start_ = (end + 1) % group_size_;
}

void SinogramLoader::copyToDevice(astra::CFloat32ProjectionData3DGPU* proj,
                                  const float* data, unsigned int start, unsigned int end) {
    unsigned int x = proj->getDetectorColCount();
    unsigned int y = end - start + 1;
    unsigned int z = proj->getDetectorRowCount();

    cudaMemcpy3DParms p;
    p.srcArray = nullptr;
    p.srcPos = make_cudaPos(0, 0, 0);
    p.srcPtr = make_cudaPitchedPtr((void *) data, x * sizeof(float), x, y);
    assert(proj->getHandle().d->arr != nullptr);
    p.dstArray = proj->getHandle().d->arr;
    p.dstPos = make_cudaPos(0, start, 0);
    p.dstPtr = make_cudaPitchedPtr(nullptr, 0, 0, 0);;
    p.extent = make_cudaExtent(x, y, z);
    p.kind = cudaMemcpyHostToDevice;

    if (!checkCudaError(cudaMemcpy3DAsync(&p, stream_->d))) {
        spdlog::error("Failed to copy sinogram data ({} - {}) from CPU to GPU", start, end);
    }
}

} // recastx::recon