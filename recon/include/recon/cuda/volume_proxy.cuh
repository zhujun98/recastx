/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_VOLUME_PROXY_H
#define RECON_CUDA_VOLUME_PROXY_H

#include "recon/cuda/buffer.cuh"

namespace recastx::recon {

class VolumeProxy {

    VolumeBuffer buffer_;

public:

    VolumeProxy();

    ~VolumeProxy();

    bool prepareBuffer() {
        return buffer_.prepare();
    }

    bool fetchBuffer(int timeout) {
        return buffer_.fetch(timeout);
    }

    void reshapeBuffer(VolumeBuffer::ShapeType shape) {
        buffer_.resize(shape);
    }

    [[nodiscard]] VolumeBuffer::ShapeType shape() const { return buffer_.shape(); }

    [[nodiscard]] ProDtype* buffer() { return buffer_.back().data(); }
    [[nodiscard]] const ProDtype* data() const { return buffer_.front().data(); }
};

} // recastx::recon

#endif // RECON_CUDA_VOLUME_PROXY_H