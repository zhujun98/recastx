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

    struct Data3D {
        const ProDtype* ptr;
        size_t x;
        size_t y;
        size_t z;
    };

    VolumeProxy();

    ~VolumeProxy();

    bool prepareBuffer() {
        return buffer_.prepare();
    }

    Data3D fetchData(int timeout) {
        bool has_data = buffer_.fetch(timeout);
        if (has_data) {
            const auto* ptr = buffer_.front().data();
            auto [x, y, z] = buffer_.shape();
            return {ptr, x, y, z};
        }
        return {nullptr, 0, 0, 0};
    }

    void reshapeBuffer(VolumeBuffer::ShapeType shape) {
        buffer_.resize(shape);
    }

    [[nodiscard]] VolumeBuffer::ShapeType shape() const { return buffer_.shape(); }

    [[nodiscard]] ProDtype* buffer() { return buffer_.back().data(); }
};

} // recastx::recon

#endif // RECON_CUDA_VOLUME_PROXY_H