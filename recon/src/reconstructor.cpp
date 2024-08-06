/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Reconstruction algorithms are originally from https://github.com/cicwi/RECAST3D.git.
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"
#include "common/scoped_timer.hpp"
#include "recon/cuda/memory.cuh"
#include "recon/cuda/sinogram_proxy.cuh"
#include "recon/cuda/volume_proxy.cuh"

namespace recastx::recon {

namespace details {

std::string astraInfo(const astra::CProjectionGeometry3D& geom) {
    auto ss = std::stringstream();

    ss << "num_rows = " << geom.getDetectorRowCount() << ", "
       << "num_cols = " << geom.getDetectorColCount() << ", "
       << "num_projections = " << geom.getProjectionCount();
    
    return ss.str();
}

}

// class Reconstructor

AstraReconstructor::AstraReconstructor(const ProjectionGeometry& p_geom, 
                                       const VolumeGeometry& s_geom, 
                                       const VolumeGeometry& v_geom)
    : Reconstructor(),
      sino_proxy_(new SinogramProxy(p_geom.angles.size())),
      volume_proxy_(new VolumeProxy),
      slice_recon_(s_geom),
      volume_recon_(v_geom) {
    volume_proxy_->reshapeBuffer({v_geom.col_count, v_geom.row_count, v_geom.slice_count});
}

AstraReconstructor::~AstraReconstructor() = default;

void AstraReconstructor::uploadSinograms(int buffer_idx) {
    spdlog::debug("Copying sinogram to GPU buffer {}", buffer_idx);

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Uploading sinograms to GPU");
#endif

    sino_proxy_->copyToDevice(data_[buffer_idx].get());
}

bool AstraReconstructor::tryPrepareSinoBuffer(int timeout) {
    return sino_proxy_->tryPrepareBuffer(timeout);
}

bool AstraReconstructor::fetchSinoBuffer(int timeout) {
    return sino_proxy_->fetchBuffer(timeout);
}

void AstraReconstructor::reshapeSinoBuffer(std::array<size_t, 3> shape) {
    sino_proxy_->reshapeBuffer(shape);
}

ProDtype* AstraReconstructor::sinoBuffer() {
    return sino_proxy_->buffer();
}

bool AstraReconstructor::prepareVolumeBuffer() {
    return volume_proxy_->prepareBuffer();
}

Reconstructor::Data3D AstraReconstructor::fetchVolumeData(int timeout) const {
    bool has_data = volume_proxy_->fetchBuffer(timeout);
    if (has_data) {
        const auto* ptr = volume_proxy_->data();
        auto [x, y, z] = volume_proxy_->shape();
        return {ptr, x, y, z};
    }
    return {nullptr, 0, 0, 0};
}

// class ParallelBeamReconstructor

ParallelBeamReconstructor::ParallelBeamReconstructor(const ProjectionGeometry& p_geom,
                                                     const VolumeGeometry& s_geom,
                                                     const VolumeGeometry& v_geom,
                                                     bool double_buffering)
        : AstraReconstructor(p_geom, s_geom, v_geom) {
    uint32_t col_count = p_geom.col_count;
    uint32_t row_count = p_geom.row_count;
    auto& angles = p_geom.angles;
    size_t angle_count = angles.size();

    bool vec_geometry = false;
    if (!vec_geometry) {
        auto geom = astra::CParallelProjectionGeometry3D(
            angle_count, row_count, col_count, p_geom.pixel_width, p_geom.pixel_height, angles.data());

        s_geom_ = utils::proj_to_vec(&geom);
        v_geom_ = utils::proj_to_vec(&geom);

    } else {
        auto par_projs = utils::list_to_par_projections(angles);

        s_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            angle_count, row_count, col_count, par_projs.data());
        v_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            angle_count, row_count, col_count, par_projs.data());
    }

    spdlog::info("[Init] - Projection geometry: {}", details::astraInfo(*s_geom_));

    vectors_ = std::vector<astra::SPar3DProjection>(
        s_geom_->getProjectionVectors(), s_geom_->getProjectionVectors() + angle_count);
    original_vectors_ = vectors_;
    vec_buf_ = vectors_;

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    auto buf = std::vector<float>(col_count * angle_count * row_count, 0.0f);
    for (int i = 0; i < (double_buffering ? 2 : 1); ++i) {
        mem_.emplace_back(col_count, angle_count, row_count);
        data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            s_geom_.get(), mem_[i].handle()));

        slice_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), data_[i].get(), slice_recon_.data()));
        volume_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), data_[i].get(), volume_recon_.data()));

        spdlog::info("[Init] - Allocated GPU memory for sinogram buffer {}: {:.1f} MB",
                     i, buf.size() * sizeof(float) / static_cast<double>(1024 * 1024));
    }
}

void ParallelBeamReconstructor::reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Reconstructing slice");
#endif

    auto k = slice_recon_.getWindowMaxX();

    auto [delta, rot, scale] = utils::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    int angle_count = s_geom_->getProjectionCount();
    int num_rows = s_geom_->getDetectorRowCount();
    int num_cols = s_geom_->getDetectorColCount();
    for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] : vectors_) {
        auto r = Eigen::Vector3f(rx, ry, rz);
        auto d = Eigen::Vector3f(dx, dy, dz);
        auto px = Eigen::Vector3f(pxx, pxy, pxz);
        auto py = Eigen::Vector3f(pyx, pyy, pyz);

        d += 0.5f * (num_cols * px + num_rows * py);
        r = scale.cwiseProduct(rot * r);
        d = scale.cwiseProduct(rot * (d + delta));
        px = scale.cwiseProduct(rot * px);
        py = scale.cwiseProduct(rot * py);
        d -= 0.5f * (num_cols * px + num_rows * py);

        vec_buf_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2], px[0], px[1], px[2], py[0], py[1], py[2]};
        ++i;
    }

    s_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        angle_count, num_rows, num_cols, vec_buf_.data());

    spdlog::debug("Reconstructing slice with buffer index: {}", buffer_idx);
    data_[buffer_idx]->changeGeometry(s_geom_.get());
    slice_algo_[buffer_idx]->run();
    slice_recon_.copySlice(buffer.data());
}

void ParallelBeamReconstructor::reconstructVolume(int buffer_idx) {

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Reconstructing volume");
#endif

    spdlog::debug("Reconstructing volume with buffer index: {}", buffer_idx);
    data_[buffer_idx]->changeGeometry(v_geom_.get());
    volume_algo_[buffer_idx]->run();
    volume_recon_.copyVolume(volume_proxy_->buffer());
}

// class ConeBeamReconstructor

ConeBeamReconstructor::ConeBeamReconstructor(const ProjectionGeometry& p_geom,
                                             const VolumeGeometry& s_geom,
                                             const VolumeGeometry& v_geom,
                                             bool double_buffering)
        : AstraReconstructor(p_geom, s_geom, v_geom) {

    uint32_t col_count = p_geom.col_count;
    uint32_t row_count = p_geom.row_count;
    auto& angles = p_geom.angles;
    size_t angle_count = angles.size();

    bool vec_geometry = false;
    if (!vec_geometry) {
        // TODO: should detector_size be (Height, Width)?
        auto geom = astra::CConeProjectionGeometry3D(
            angle_count, row_count, col_count, 
            p_geom.pixel_width, p_geom.pixel_height, angles.data(), 
            p_geom.source2origin, p_geom.origin2detector);

        s_geom_ = utils::proj_to_vec(&geom);
        v_geom_ = utils::proj_to_vec(&geom);
    } else {
        auto cone_projs = utils::list_to_cone_projections(row_count, col_count, angles);

        s_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            angle_count, row_count, col_count, cone_projs.data());
        v_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            angle_count, row_count, col_count, cone_projs.data());
    }
    
    spdlog::info("[Init] - projection geometry: {}", details::astraInfo(*s_geom_));

    vectors_ = std::vector<astra::SConeProjection>(
        s_geom_->getProjectionVectors(), s_geom_->getProjectionVectors() + angle_count);

    vec_buf_ = vectors_;

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    auto buf = std::vector<float>(col_count * angle_count * row_count, 0.0f);
    for (int i = 0; i < (double_buffering ? 2 : 1); ++i) {
        mem_.emplace_back(col_count, angle_count, row_count);
        data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            s_geom_.get(), mem_[i].handle()));

        slice_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), data_[i].get(), slice_recon_.data()));
        volume_algo_.push_back(
            std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), data_[i].get(), volume_recon_.data()));
    }
}

void ConeBeamReconstructor::reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {
    auto k = slice_recon_.getWindowMaxX();

    auto [delta, rot, scale] = utils::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] : vectors_) {
        auto s = Eigen::Vector3f(rx, ry, rz);
        auto d = Eigen::Vector3f(dx, dy, dz);
        auto t1 = Eigen::Vector3f(pxx, pxy, pxz);
        auto t2 = Eigen::Vector3f(pyx, pyy, pyz);

        s = scale.cwiseProduct(rot * (s + delta));
        d = scale.cwiseProduct(rot * (d + delta));
        t1 = scale.cwiseProduct(rot * t1);
        t2 = scale.cwiseProduct(rot * t2);

        vec_buf_[i] = {s[0],  s[1],  s[2],  d[0],  d[1],  d[2], t1[0], t1[1], t1[2], t2[0], t2[1], t2[2]};
        ++i;
    }

    int angle_count = s_geom_->getProjectionCount();
    int num_rows = s_geom_->getDetectorRowCount();
    int num_cols = s_geom_->getDetectorColCount();
    s_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
        angle_count, num_rows, num_cols, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    data_[buffer_idx]->changeGeometry(s_geom_.get());
    slice_algo_[buffer_idx]->run();
    slice_recon_.copySlice(buffer.data());
}

void ConeBeamReconstructor::reconstructVolume(int buffer_idx) {
    data_[buffer_idx]->changeGeometry(v_geom_.get());
    volume_algo_[buffer_idx]->run();
    volume_recon_.copyVolume(volume_proxy_->buffer());
}

std::vector<float> ConeBeamReconstructor::fdk_weights() {
    int angle_count = s_geom_->getProjectionCount();
    int num_rows = s_geom_->getDetectorRowCount();
    int num_cols = s_geom_->getDetectorColCount();
    auto result = std::vector<float>(angle_count * num_rows * num_cols, 0.0f);

    int i = 0;
    for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] : vectors_) {
        auto s = Eigen::Vector3f(rx, ry, rz);
        auto d = Eigen::Vector3f(dx, dy, dz);
        auto t1 = Eigen::Vector3f(pxx, pxy, pxz);
        auto t2 = Eigen::Vector3f(pyx, pyy, pyz);

        // FIXME uncentered projections
        // rho should be the distance between the source and the detector plane
        // which is not equal to (d - s).norm() for uncentered projections
        auto rho = (d - s).norm();
        for (int r = 0; r < num_rows; ++r) {
            for (int c = 0; c < num_cols; ++c) {
                auto y = d + r * t2 + c * t1;
                auto denum = (y - s).norm();
                result[(i * num_cols * num_rows) + (r * num_cols) + c] = rho / denum;
            }
        }

        ++i;
    }

    return result;
}


std::unique_ptr<Reconstructor> 
AstraReconstructorFactory::create(ProjectionGeometry proj_geom,
                                  VolumeGeometry slice_geom, 
                                  VolumeGeometry volume_geom,
                                  bool double_buffering) {
    if (proj_geom.beam_shape == BeamShape::CONE) {
        return std::make_unique<ConeBeamReconstructor>(proj_geom, slice_geom, volume_geom, double_buffering);
    }
    return std::make_unique<ParallelBeamReconstructor>(proj_geom, slice_geom, volume_geom, double_buffering);
}

} // namespace recastx::recon
