/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_UTILS_H
#define RECON_CUDA_UTILS_H

#include <cuda_runtime.h>

#include <spdlog/spdlog.h>

namespace astraCUDA3d {

    struct SMemHandle3D_internal
    {
        cudaPitchedPtr ptr;
        cudaArray *arr;
        unsigned int nx;
        unsigned int ny;
        unsigned int nz;
    };

} // astraCUDA3d

namespace recastx::recon {

namespace details {

inline bool checkCuda(cudaError_t result, char const *const func, const char *const file, int const line) {
    if (result != cudaSuccess) {
        spdlog::error("CUDA error at {}:{} code={}({}) \"{}\" \n", file, line,
                      static_cast<unsigned int>(result), cudaGetErrorName(result), func);
        return false;
    }
    return true;
}

} // namespace details

#define checkCudaError(val) details::checkCuda((val), #val, __FILE__, __LINE__)

inline std::pair<size_t, size_t> queryGPUMemory() {
    size_t free, total;
    cudaError_t err = cudaMemGetInfo(&free, &total);
    if (err != cudaSuccess)
        return {0, 0};
    return {free, total};
}

} // namespace recastx::recon

#endif // RECON_CUDA_UTILS_H