/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
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
#include "reconstructor_interface.hpp"

namespace recastx::recon {

namespace details {
    std::string astraInfo(const astra::CConeVecProjectionGeometry3D& x);
    std::string astraInfo(const astra::CVolumeGeometry3D& x);
}

class Uploader {

    size_t start_;
    size_t group_size_;

  public:

    explicit Uploader(size_t group_size);

    void upload(astra::CFloat32ProjectionData3DGPU* dst, const float* src, size_t n);
};

class AstraReconstructor : public Reconstructor {

protected:

    std::vector<Uploader> uploader_;
    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>> p_data_;
    std::vector<astraCUDA3d::MemHandle3D> p_mem_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;

    std::unique_ptr<astra::CVolumeGeometry3D> s_geom_;
    astraCUDA3d::MemHandle3D s_mem_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> s_data_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> s_algo_;

    std::unique_ptr<astra::CVolumeGeometry3D> v_geom_;
    astraCUDA3d::MemHandle3D v_mem_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> v_data_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> v_algo_;

    std::unique_ptr<astra::CVolumeGeometry3D> makeVolumeGeometry(const VolumeGeometry& geom);
    astraCUDA3d::MemHandle3D makeVolumeGeometryMemHandle(const VolumeGeometry& geom);

public:

    AstraReconstructor(const ProjectionGeometry& p_geom, 
                       const VolumeGeometry& s_geom, 
                       const VolumeGeometry& v_geom,
                       bool double_buffering);

    virtual ~AstraReconstructor();

    void uploadSinograms(int buffer_idx, const float* data, size_t n);
};

class ParallelBeamReconstructor : public AstraReconstructor {

    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> s_p_geom_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> v_p_geom_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> original_vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;

public:

    ParallelBeamReconstructor(size_t col_count, 
                              size_t row_count, 
                              const ProjectionGeometry& p_geom, 
                              const VolumeGeometry& s_geom, 
                              const VolumeGeometry& v_geom, 
                              bool double_buffer);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) override;

};

class ConeBeamReconstructor : public AstraReconstructor {

    std::unique_ptr<astra::CConeVecProjectionGeometry3D> s_p_geom_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> v_p_geom_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    ConeBeamReconstructor(size_t col_count, 
                          size_t row_count, 
                          const ProjectionGeometry& p_geom, 
                          const VolumeGeometry& s_geom, 
                          const VolumeGeometry& v_geom, 
                          bool double_buffer);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) override;

    std::vector<float> fdk_weights();
};

class AstraReconstructorFactory : public ReconstructorFactory {

public:

    std::unique_ptr<Reconstructor> create(size_t col_count, 
                                          size_t row_count, 
                                          ProjectionGeometry proj_geom, 
                                          VolumeGeometry slice_geom, 
                                          VolumeGeometry preview_geom,
                                          bool double_buffering) override;

};


} // namespace recastx::recon

#endif // RECON_RECONSTRUCTOR_H