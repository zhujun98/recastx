/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_SINOGRAM_LOADER_H
#define RECON_CUDA_SINOGRAM_LOADER_H

#include "astra/Float32ProjectionData3DGPU.h"


namespace recastx::recon {

class Stream;

class SinogramLoader {

    size_t start_;
    size_t group_size_;

    std::unique_ptr<Stream> stream_;

    void copyToDevice(astra::CFloat32ProjectionData3DGPU *proj,
                      const float *data, unsigned int y_min, unsigned int y_max);

public:

    explicit SinogramLoader(size_t group_size);

    ~SinogramLoader();

    void load(astra::CFloat32ProjectionData3DGPU *dst, const float *src, size_t n);
};

} // recastx::recon

#endif // RECON_CUDA_SINOGRAM_LOADER_H