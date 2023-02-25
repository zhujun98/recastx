#ifndef SLICERECON_UTILS_H
#define SLICERECON_UTILS_H

#include <memory>
#include <numeric>
#include <type_traits>

#include <Eigen/Eigen>

#ifndef ASTRA_CUDA
#define ASTRA_CUDA
#endif

#include "astra/ConeProjectionGeometry3D.h"
#include "astra/ConeVecProjectionGeometry3D.h"
#include "astra/Float32ProjectionData3DGPU.h"
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/VolumeGeometry3D.h"

#include "common/config.hpp"
#include "tensor.hpp"

namespace tomcat::recon::utils {

std::unique_ptr<astra::CParallelVecProjectionGeometry3D>
parallel_beam_to_vec(astra::CParallelProjectionGeometry3D* parallel_geom);

std::vector<astra::SPar3DProjection>
list_to_par_projections(const std::vector<float>& vectors);

std::vector<astra::SConeProjection>
list_to_cone_projections(int rows, int cols, const std::vector<float>& vectors);

std::unique_ptr<astra::CParallelVecProjectionGeometry3D>
proj_to_vec(astra::CParallelProjectionGeometry3D* parallel_geom);

std::unique_ptr<astra::CConeVecProjectionGeometry3D>
proj_to_vec(astra::CConeProjectionGeometry3D* cone_geom);

std::tuple<Eigen::Vector3f, Eigen::Matrix3f, Eigen::Vector3f>
slice_transform(Eigen::Vector3f base, Eigen::Vector3f axis_1,
                Eigen::Vector3f axis_2, float k);

} // tomcat::recon::utils

#endif // SLICERECON_UTILS_H