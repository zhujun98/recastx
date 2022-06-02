#pragma once

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

#include "data_types.hpp"


namespace slicerecon {

class Solver {

protected:

    int rows_;
    int cols_;
    int projections_;

    int preview_size_;
    int slice_size_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_;
    astraCUDA3d::MemHandle3D vol_handle_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_small_;
    astraCUDA3d::MemHandle3D vol_handle_small_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_small_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algs_small_;

    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>> proj_data_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algs_;
    std::vector<astraCUDA3d::MemHandle3D> proj_handles_;

public:

    Solver(int rows, 
           int cols, 
           int projections,
           const std::array<float, 3>& volume_min_point, 
           const std::array<float, 3>& volume_max_point,
           int preview_size,
           int slice_size);

    virtual ~Solver();

    virtual slice_data reconstructSlice(orientation x, int buffer_idx) = 0;
    virtual void reconstructPreview(std::vector<float>& preview_buffer, int buffer_idx) = 0;

    // returns true if we want to trigger a re-reconstruction
    virtual bool
    parameterChanged(std::string param, std::variant<float, std::string, bool> value) {
        return false;
    }

    void uploadProjections(int buffer_idx, const std::vector<float>& sino, int begin, int end);
};

class ParallelBeamSolver : public Solver {

    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_small_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> original_vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;
    float tilt_translate_ = 0.0f;
    float tilt_rotate_ = 0.0f;

public:

    ParallelBeamSolver(int rows, 
                       int cols,
                       int projections,
                       std::vector<float> angles,
                       const std::array<float, 3>& volume_min_point, 
                       const std::array<float, 3>& volume_max_point,
                       int preview_size,
                       int slice_size,
                       bool vec_geometry,
                       ReconstructMode recon_mode);
    // FIXME ~solver clean up

    slice_data reconstructSlice(orientation x, int buffer_idx) override;
    void reconstructPreview(std::vector<float>& preview_buffer, int buffer_idx) override;

    bool parameterChanged(std::string param, std::variant<float, std::string, bool> value) override;
};

class ConeBeamSolver : public Solver {

    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_small_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    ConeBeamSolver(int rows, 
                   int cols,
                   int projections,
                   std::vector<float> angles,
                   const std::array<float, 3>& volume_min_point, 
                   const std::array<float, 3>& volume_max_point,
                   int preview_size,
                   int slice_size,
                   bool vec_geometry,
                   const std::array<float, 2>& detector_size,
                   float source_origin,
                   float origin_det,
                   ReconstructMode recon_mode);
    // FIXME ~solver clean up

    slice_data reconstructSlice(orientation x, int buffer_idx) override;
    void reconstructPreview(std::vector<float>& preview_buffer, int buffer_idx) override;
    std::vector<float> fdk_weights();
};

} // slicerecon