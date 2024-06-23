/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <boost/shared_ptr.hpp>

#include "recon/cuda/utils.cuh"
#include "recon/cuda/recon3d.cuh"
#include "recon/cuda/memory.cuh"
#include "recon/cuda/stream.cuh"

namespace recastx::recon {

AstraReconstructable::AstraReconstructable(const VolumeGeometry& geom)
        : geom_(new astra::CVolumeGeometry3D(
                    geom.col_count, geom.row_count, geom.slice_count,
                    geom.min_x, geom.min_y, geom.min_z, geom.max_x, geom.max_y, geom.max_z)),
          mem_(new AstraMemHandle(geom.col_count, geom.row_count, geom.slice_count)),
          data_(new astra::CFloat32VolumeData3DGPU(geom_.get(), mem_->handle())),
          stream_(new Stream) {

    spdlog::info("[Init] - Volume geometry: shape {} x {} x {}, x [{}, {}], y [{}, {}], z [{}, {}]",
                 geom_->getGridRowCount(), geom_->getGridColCount(), geom_->getGridSliceCount(),
                 geom_->getWindowMinX(), geom_->getWindowMaxX(),
                 geom_->getWindowMinY(), geom_->getWindowMaxY(),
                 geom_->getWindowMinZ(), geom_->getWindowMaxZ());
}

AstraReconstructable::~AstraReconstructable() = default;

void AstraReconstructable::copySlice(float* buffer) {
    unsigned int x = geom_->getGridColCount();
    unsigned int y = geom_->getGridRowCount();
    assert(x == y);
    copyFromDevice(buffer, mem_.get(), x, y, 1, x);
}

void AstraReconstructable::copyVolume(float* buffer) {
    unsigned int x = geom_->getGridColCount();
    unsigned int y = geom_->getGridRowCount();
    unsigned int z = geom_->getGridSliceCount();
    assert(x == y);
    assert(x == z);
    copyFromDevice(buffer, mem_.get(), x, y, z, x);
}

bool AstraReconstructable::copyFromDevice(float* dst, const AstraMemHandle* src,
                                          unsigned int x, unsigned int y, unsigned int z, unsigned int pitch) {
    spdlog::debug("Copying {} x {} x {} from GPU", x, y, z);

    const auto& handle = src->handle();
    cudaMemcpy3DParms p;

    p.srcPos = make_cudaPos(0, 0, 0);
    if (handle.d->ptr.ptr != nullptr) {
        p.srcArray = nullptr;
        p.srcPtr = handle.d->ptr;
        p.extent = make_cudaExtent(x * sizeof(float), y, z);
    } else {
        p.srcArray = handle.d->arr;
        p.srcPtr = make_cudaPitchedPtr(nullptr, 0, 0, 0);
        p.extent = make_cudaExtent(x, y, z);
    }

    p.dstArray = nullptr;
    p.dstPos = make_cudaPos(0 * sizeof(float), 0, 0);
    p.dstPtr = make_cudaPitchedPtr((void*)dst, pitch * sizeof(float), x * sizeof(float), y);

    p.kind = cudaMemcpyDeviceToHost;

    return checkCudaError(cudaMemcpy3DAsync(&p, stream_->d));
}

} // namespace recastx::recon