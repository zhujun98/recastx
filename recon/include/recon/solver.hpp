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

#include "tomcat/tomcat.hpp"

namespace tomcat::recon {

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

    /**
     * @brief Construct a new Solver:: Solver object
     * 
     * @param rows: Number of rows of the detector.
     * @param cols: Number of columns of the detector.
     * @param projections: Number of projection angles. 
     * @param volume_min_point: Minimal (X, Y, Z)-coordinate in the volume window.
     * @param volume_max_point: Maximal (X, Y, Z)-coordinate in the volume window. 
     * @param preview_size: Size of the cubic reconstructed volume for preview.
     * @param slice_size: Size of the square reconstructed slice in pixels. 
     */
    Solver(int rows, 
           int cols, 
           int projections,
           const std::array<float, 3>& volume_min_point, 
           const std::array<float, 3>& volume_max_point,
           int preview_size,
           int slice_size);

    virtual ~Solver();

    virtual void reconstructSlice(std::vector<float>& slice_buffer, 
                                  Orientation x, 
                                  int buffer_idx) = 0;

    virtual void reconstructPreview(std::vector<float>& preview_buffer, 
                                    int buffer_idx) = 0;

    // returns true if we want to trigger a re-reconstruction
    virtual bool
    parameterChanged(std::string param, std::variant<float, std::string, bool> value) {
        return false;
    }

    void uploadSinograms(int buffer_idx, const std::vector<float>& sino, int begin, int end);
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

    /**
     * @brief Construct a new Parallel Beam Solver:: Parallel Beam Solver object
     * 
     * @param rows: Number of rows of the detector.
     * @param cols: Number of columns of the detector.
     * @param angles: An array of projection angles.
     * @param volume_min_point: Minimal (X, Y, Z)-coordinate in the volume window.
     * @param volume_max_point: Maximal (X, Y, Z)-coordinate in the volume window. 
     * @param preview_size: Size of the cubic reconstructed volume for preview.
     * @param slice_size: Size of the square reconstructed slice in pixels. 
     * @param vec_geometry 
     * @param detector_size: (Height, Width) of a detector cell, in unit lengths.
     */
    ParallelBeamSolver(int rows, 
                       int cols,
                       std::vector<float> angles,
                       const std::array<float, 3>& volume_min_point, 
                       const std::array<float, 3>& volume_max_point,
                       int preview_size,
                       int slice_size,
                       bool vec_geometry,
                       const std::array<float, 2>& detector_size);
    // FIXME ~solver clean up

    void reconstructSlice(std::vector<float>& slice_buffer, 
                          Orientation x, 
                          int buffer_idx) override;

    void reconstructPreview(std::vector<float>& preview_buffer, 
                            int buffer_idx) override;

    bool parameterChanged(std::string param, 
                          std::variant<float, std::string, bool> value) override;
};

class ConeBeamSolver : public Solver {

    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_;
    std::unique_ptr<astra::CConeVecProjectionGeometry3D> proj_geom_small_;
    std::vector<astra::SConeProjection> vectors_;
    std::vector<astra::SConeProjection> vec_buf_;

public:

    /**
     * @brief Construct a new Cone Beam Solver:: Cone Beam Solver object
     * 
     * @param rows: Number of rows of the detector.
     * @param cols: Number of columns of the detector.
     * @param angles: An array of projection angles. 
     * @param volume_min_point: Minimal (X, Y, Z)-coordinate in the volume window.
     * @param volume_max_point: Maximal (X, Y, Z)-coordinate in the volume window. 
     * @param preview_size: Size of the cubic reconstructed volume for preview.
     * @param slice_size: Size of the square reconstructed slice in pixels.
     * @param vec_geometry 
     * @param detector_size: (Height, Width) of a detector cell, in unit lengths.
     * @param source_origin: Distance from the origin of the coordinate system to the source.
     * @param origin_det: Distance from the origin of the coordinate system to the detector 
     *                    (i.e., the distance between the origin and its orthogonal projection
     *                    onto the detector array).
     */
    ConeBeamSolver(int rows, 
                   int cols,
                   std::vector<float> angles,
                   const std::array<float, 3>& volume_min_point, 
                   const std::array<float, 3>& volume_max_point,
                   int preview_size,
                   int slice_size,
                   bool vec_geometry,
                   const std::array<float, 2>& detector_size,
                   float source_origin,
                   float origin_det);
    // FIXME ~solver clean up

    void reconstructSlice(std::vector<float>& slice_buffer, 
                          Orientation x, 
                          int buffer_idx) override;

    void reconstructPreview(std::vector<float>& preview_buffer, 
                            int buffer_idx) override;

    std::vector<float> fdk_weights();
};

} // tomcat::recon