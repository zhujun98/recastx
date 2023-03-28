#ifndef RECON_RECONSTRUCTOR_H
#define RECON_RECONSTRUCTOR_H

#include <memory>
#include <variant>

#ifndef ASTRA_CUDA
#define ASTRA_CUDA
#endif

#include "astra/ConeProjectionGeometry3D.h"
#include "astra/ConeVecProjectionGeometry3D.h"
#include "astra/CudaBackProjectionAlgorithm3D.h"
#include "astra/CudaProjector3D.h"
#include "astra/Float32Data3DGPU.h"
#include "astra/Float32ProjectionData3DGPU.h"
#include "astra/Float32VolumeData3DGPU.h"
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/VolumeGeometry3D.h"

#include "common/config.hpp"
#include "tensor.hpp"

namespace recastx::recon {

namespace details {
    std::string astraInfo(const astra::CConeVecProjectionGeometry3D& x);
    std::string astraInfo(const astra::CVolumeGeometry3D& x);
}

class Reconstructor {

protected:

    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>> proj_data_;
    std::vector<astraCUDA3d::MemHandle3D> gpu_mem_proj_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_slice_;
    astraCUDA3d::MemHandle3D vol_mem_slice_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_slice_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algo_slice_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_preview_;
    astraCUDA3d::MemHandle3D vol_mem_preview_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_preview_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algo_preview_;

    std::unique_ptr<astra::CVolumeGeometry3D> makeVolumeGeometry(const VolumeGeometry& geom);
    astraCUDA3d::MemHandle3D makeVolumeGeometryMemHandle(const VolumeGeometry& geom);

public:

    Reconstructor(VolumeGeometry slice_geom, VolumeGeometry preview_geom);

    virtual ~Reconstructor();

    virtual void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) = 0;

    virtual void reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) = 0;

    void uploadSinograms(int buffer_idx, const float* data, int begin, int end);
};

class ParallelBeamReconstructor : public Reconstructor {

    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_slice_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_preview_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> original_vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;

public:

    ParallelBeamReconstructor(ProjectionGeometry proj_geom, VolumeGeometry slice_geom,
                              VolumeGeometry preview_geom);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) override;

};

class ConeBeamReconstructor : public Reconstructor {

    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_slice_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_preview_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    ConeBeamReconstructor(ProjectionGeometry proj_geom, 
                          VolumeGeometry slice_geom,
                          VolumeGeometry preview_geom);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) override;

    std::vector<float> fdk_weights();
};

} // namespace recastx::recon

#endif // RECON_RECONSTRUCTOR_H