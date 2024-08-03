/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_SINOGRAM_H
#define RECON_CUDA_SINOGRAM_H

#include "astra/Float32ProjectionData3DGPU.h"

#include "recon/cuda/buffer.cuh"

namespace recastx::recon {

class Stream;

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

#endif // RECON_CUDA_SINOGRAM_H