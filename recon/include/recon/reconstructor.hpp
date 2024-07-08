/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/VolumeGeometry3D.h"

#include "common/config.hpp"
#include "reconstructor_interface.hpp"
#include "cuda/recon3d.cuh"

namespace recastx::recon {

class AstraMemHandleArray;
class SinogramManager;

class AstraReconstructor : public Reconstructor {

protected:

    std::unique_ptr<SinogramManager> sino_manager_;

    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>> data_;
    std::vector<AstraMemHandleArray> mem_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;

    AstraReconstructable slice_recon_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> slice_algo_;

    AstraReconstructable volume_recon_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> volume_algo_;

public:

    AstraReconstructor(const ProjectionGeometry& p_geom, 
                       const VolumeGeometry& s_geom, 
                       const VolumeGeometry& v_geom);

    ~AstraReconstructor() override;

    void uploadSinograms(int buffer_idx) override;

    bool tryPrepareSinoBuffer() override;

    bool fetchSinoBuffer() override;

    void reshapeSinoBuffer(std::array<size_t, 3>) override;

    [[nodiscard]] ProDtype* sinoBuffer() override;
};

class ParallelBeamReconstructor : public AstraReconstructor {

    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> s_geom_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> v_geom_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> original_vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;

public:

    ParallelBeamReconstructor(const ProjectionGeometry& p_geom,
                              const VolumeGeometry& s_geom, 
                              const VolumeGeometry& v_geom, 
                              bool double_buffer);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructVolume(int buffer_idx, Tensor<float, 3>& buffer) override;

};

class ConeBeamReconstructor : public AstraReconstructor {

    std::unique_ptr<astra::CConeVecProjectionGeometry3D> s_geom_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> v_geom_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    ConeBeamReconstructor(const ProjectionGeometry& p_geom,
                          const VolumeGeometry& s_geom, 
                          const VolumeGeometry& v_geom, 
                          bool double_buffer);
    // FIXME ~solver clean up

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override;

    void reconstructVolume(int buffer_idx, Tensor<float, 3>& buffer) override;

    std::vector<float> fdk_weights();
};

class AstraReconstructorFactory : public ReconstructorFactory {

public:

    std::unique_ptr<Reconstructor> create(ProjectionGeometry proj_geom,
                                          VolumeGeometry slice_geom, 
                                          VolumeGeometry volume_geom,
                                          bool double_buffering) override;

};


} // namespace recastx::recon

#endif // RECON_RECONSTRUCTOR_H