#pragma once

#include <memory>
#include <numeric>

#include <Eigen/Eigen>
#include <oneapi/tbb.h>
#include <spdlog/spdlog.h>

#ifndef ASTRA_CUDA
#define ASTRA_CUDA
#endif

#include "astra/ConeProjectionGeometry3D.h"
#include "astra/ConeVecProjectionGeometry3D.h"
#include "astra/Float32ProjectionData3DGPU.h"
#include "astra/ParallelProjectionGeometry3D.h"
#include "astra/ParallelVecProjectionGeometry3D.h"
#include "astra/VolumeGeometry3D.h"

#include "tomcat/tomcat.hpp"

namespace tomcat::slicerecon::utils {

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

std::string info(const astra::CConeVecProjectionGeometry3D& x);
std::string info(const astra::CVolumeGeometry3D& x);

template<typename T>
inline void averageImages(const std::vector<T>& images, 
                          size_t pixels,
                          std::vector<float>& ret) {
    size_t n = images.size() / pixels;
    for (size_t i = 0; i < pixels; ++i) {
        float total = 0.0f;
        for (size_t j = 0; j < n; ++j) {
            total += static_cast<float>(images[pixels * j + i]);
        }
        ret[i] = total / n;
    }
}

inline void computeReciprocal(const std::vector<RawDtype>& darks,
                              const std::vector<RawDtype>& flats,
                              size_t pixels,
                              std::vector<float>& reciprocal,
                              std::vector<float>& dark_avg) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif
    averageImages<RawDtype>(darks, pixels, dark_avg);
    std::vector<float> flat_avg(dark_avg.size());
    averageImages<RawDtype>(flats, pixels, flat_avg);
    for (size_t i = 0; i < pixels; ++i) {
        if (dark_avg[i] == flat_avg[i]) {
            reciprocal[i] = 1.0f;
        } else {
            reciprocal[i] = 1.0f / (flat_avg[i] - dark_avg[i]);
        }
    }
#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Computing reciprocal took {} ms", duration/1000);
#endif
}

inline void flatField(float* data, size_t size, const ImageData& dark, const ImageData& reciprocal) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = (data[i] - dark[i]) * reciprocal[i];
    } 
}

inline void negativeLog(float* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = data[i] <= 0.0f ? 0.0f : -std::log(data[i]);
    }
}

inline std::vector<float> defaultAngles(int n) {
    auto angles = std::vector<float>(n, 0.0f);
    std::iota(angles.begin(), angles.end(), 0.0f);
    std::transform(angles.begin(), angles.end(), angles.begin(),
                   [n](auto x) { return (x * M_PI) / n; });
    return angles;
}



} // tomcat::slicerecon::utils
