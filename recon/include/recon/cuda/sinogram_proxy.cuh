/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_SINOGRAM_MANAGER_H
#define RECON_CUDA_SINOGRAM_MANAGER_H

#include "astra/Float32ProjectionData3DGPU.h"

#include "recon/cuda/buffer.cuh"

namespace recastx::recon {

class Stream;

class SinogramProxy {

    SinogramBuffer buffer_;

    size_t start_;
    size_t group_size_;

    std::unique_ptr<Stream> stream_;

    void copyToDevice(astra::CFloat32ProjectionData3DGPU *proj,
                      const float *data, unsigned int y_min, unsigned int y_max);

  public:

    explicit SinogramProxy(size_t group_size);

    ~SinogramProxy();

    void copyToDevice(astra::CFloat32ProjectionData3DGPU *dst);

    bool tryPrepareBuffer(int timeout) {
        return buffer_.tryPrepare(timeout);
    }

    bool fetchBuffer(int timeout) {
        return buffer_.fetch(timeout);
    }

    void reshapeBuffer(SinogramBuffer::ShapeType shape) {
        buffer_.resize(shape);
    }

    [[nodiscard]] ProDtype* buffer() { return buffer_.back().data(); }
};

} // recastx::recon

#endif // RECON_CUDA_SINOGRAM_MANAGER_H