/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_STREAM_H
#define RECON_CUDA_STREAM_H

#include <cuda_runtime.h>

namespace recastx::recon {

struct Stream {

    cudaStream_t d;

    Stream() { cudaStreamCreate(&d); }

    ~Stream() { cudaStreamDestroy(d); }

};

} // recastx::recon
#endif