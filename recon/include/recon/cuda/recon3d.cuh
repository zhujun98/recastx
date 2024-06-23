/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_CUDA_RECON3D_H
#define RECON_CUDA_RECON3D_H

#ifndef ASTRA_CUDA
#define ASTRA_CUDA
#endif

#include "astra/CudaBackProjectionAlgorithm3D.h"
#include "astra/CudaProjector3D.h"
#include "astra/Float32Data3DGPU.h"
#include "astra/Float32ProjectionData3DGPU.h"
#include "astra/Float32VolumeData3DGPU.h"

#include "common/config.hpp"


namespace recastx::recon {

class Stream;
class AstraMemHandle;

class AstraReconstructable {

    std::unique_ptr<astra::CVolumeGeometry3D> geom_;
    std::unique_ptr<AstraMemHandle> mem_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> data_;

    std::unique_ptr<Stream> stream_;

    bool copyFromDevice(float* dst, const AstraMemHandle* src,
                        unsigned int x, unsigned int y, unsigned int z, unsigned int pitch);

public:

    explicit AstraReconstructable(const VolumeGeometry& geom);

    ~AstraReconstructable();

    [[nodiscard]] astra::CFloat32VolumeData3DGPU* data() { return data_.get(); }

    void copySlice(float* buffer);

    void copyVolume(float* buffer);

    [[nodiscard]] float getWindowMaxX() const { return geom_->getWindowMaxX(); }
};

} // namespace recastx::recon

#endif // RECON_CUDA_RECON3D_H