#pragma once

#include <memory>

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

#include "../data_types.hpp"


namespace slicerecon {

class Solver {

protected:

    Settings param_;
    Geometry geom_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_;
    astraCUDA3d::MemHandle3D vol_handle_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_;
    std::unique_ptr<astra::CCudaProjector3D> projector_;

    std::unique_ptr<astra::CVolumeGeometry3D> vol_geom_small_;
    astraCUDA3d::MemHandle3D vol_handle_small_;
    std::unique_ptr<astra::CFloat32VolumeData3DGPU> vol_data_small_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algs_small_;

    std::vector<std::unique_ptr<astra::CFloat32ProjectionData3DGPU>> proj_datas_;
    std::vector<std::unique_ptr<astra::CCudaBackProjectionAlgorithm3D>> algs_;
    std::vector<astraCUDA3d::MemHandle3D> proj_handles_;

public:

    Solver(Settings param, Geometry geom);
    virtual ~Solver();

    virtual slice_data reconstruct_slice(orientation x, int buffer_idx) = 0;
    virtual void reconstruct_preview(std::vector<float>& preview_buffer, int buffer_idx) = 0;

    auto proj_data(int index) { return proj_datas_[index].get(); }

    // returns true if we want to trigger a re-reconstruction
    virtual bool
    parameter_changed(std::string param, std::variant<float, std::string, bool> value) {
        return false;
    }

    virtual std::vector<std::pair<std::string, std::variant<float, std::vector<std::string>, bool>>>
    parameters() {
        return {};
    }
};

class ParallelBeamSolver : public Solver {

    // Parallel specific stuff
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_;
    std::unique_ptr<astra::CParallelVecProjectionGeometry3D> proj_geom_small_;
    std::vector<astra::SPar3DProjection> vectors_;
    std::vector<astra::SPar3DProjection> original_vectors_;
    std::vector<astra::SPar3DProjection> vec_buf_;
    float tilt_translate_ = 0.0f;
    float tilt_rotate_ = 0.0f;

public:

    ParallelBeamSolver(Settings param, Geometry geom);
    // FIXME ~solver clean up

    slice_data reconstruct_slice(orientation x, int buffer_idx) override;
    void reconstruct_preview(std::vector<float>& preview_buffer, int buffer_idx) override;

    bool parameter_changed(std::string param, std::variant<float, std::string, bool> value) override;
    std::vector<std::pair<std::string,std::variant<float, std::vector<std::string>, bool>>>
    parameters() override;
};

class ConeBeamSolver : public Solver {
    // Cone specific stuff
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_small_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    ConeBeamSolver(Settings param, Geometry geom);
    // FIXME ~solver clean up

    slice_data reconstruct_slice(orientation x, int buffer_idx) override;
    void reconstruct_preview(std::vector<float>& preview_buffer, int buffer_idx) override;
    std::vector<float> fdk_weights();
};

} // slicerecon