#include <spdlog/spdlog.h>

#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"

namespace tomcat::recon {

namespace details {

std::string astraInfo(const astra::CParallelVecProjectionGeometry3D& geom) {
    auto ss = std::stringstream();

    ss << "parallel beam, "
       << "num_rows = " << geom.getDetectorRowCount() << ", " 
       << "num_cols = " << geom.getDetectorColCount() << ", "
       << "num_projections = " << geom.getProjectionCount();

    return ss.str();
}

std::string astraInfo(const astra::CConeVecProjectionGeometry3D& geom) {
    auto ss = std::stringstream();

    ss << "cone beam, "
       << "num_rows = " << geom.getDetectorRowCount() << ", " 
       << "num_cols = " << geom.getDetectorColCount() << ", "
       << "num_projections = " << geom.getProjectionCount();
    
    return ss.str();
}

std::string astraInfo(const astra::CVolumeGeometry3D& geom) {
    auto ss = std::stringstream();

    ss << "Min = [" << geom.getWindowMinX() << ", " << geom.getWindowMinY() << ", "
       << geom.getWindowMinZ() << "], ";
    ss << "Max = [" << geom.getWindowMaxX() << ", " << geom.getWindowMaxY() << ", "
       << geom.getWindowMaxZ() << "], ";
    ss << "Shape = [" << geom.getGridRowCount() << ", " << geom.getGridColCount()
       << ", " << geom.getGridSliceCount() << "]";

    return ss.str();
}

}

// class Reconstructor

Reconstructor::Reconstructor(VolumeGeometry slice_geom, VolumeGeometry preview_geom) {
    vol_geom_slice_ = makeVolumeGeometry(slice_geom);
    vol_mem_slice_ = makeVolumeGeometryMemHandle(slice_geom);
    vol_data_slice_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
        vol_geom_slice_.get(), vol_mem_slice_);
    spdlog::info("- Slice volume geometry: {}", details::astraInfo(*vol_geom_slice_));
    
    vol_geom_preview_ = makeVolumeGeometry(preview_geom);
    vol_mem_preview_ = makeVolumeGeometryMemHandle(preview_geom);
    vol_data_preview_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
        vol_geom_preview_.get(), vol_mem_preview_);
    spdlog::info("- Preview volume geometry: {}", details::astraInfo(*vol_geom_preview_));
}

Reconstructor::~Reconstructor() {
    spdlog::info("Deconstructing Reconstructor and freeing GPU memory ...");

    astraCUDA3d::freeGPUMemory(vol_mem_slice_);
    astraCUDA3d::freeGPUMemory(vol_mem_preview_);
    for (auto& handle : gpu_mem_proj_) astraCUDA3d::freeGPUMemory(handle);
}

void Reconstructor::uploadSinograms(int buffer_idx, 
                                    const float* data,
                                    int begin, 
                                    int end) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    astra::uploadMultipleProjections(proj_data_[buffer_idx].get(), data, begin, end);

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Uploading sinograms to GPU took {} ms", duration / 1000);
#endif
}

std::unique_ptr<astra::CVolumeGeometry3D>
Reconstructor::makeVolumeGeometry(const VolumeGeometry& geom) {
    return std::make_unique<astra::CVolumeGeometry3D>(
        geom.col_count, geom.row_count, geom.slice_count,
        geom.min_x, geom.min_y, geom.min_z, geom.max_x, geom.max_y, geom.max_z); 
}

astraCUDA3d::MemHandle3D
Reconstructor::makeVolumeGeometryMemHandle(const VolumeGeometry& geom) {
    return  astraCUDA3d::allocateGPUMemory(
        geom.col_count, geom.row_count, geom.slice_count, astraCUDA3d::INIT_ZERO);
}


// class ParallelBeamReconstructor

ParallelBeamReconstructor::ParallelBeamReconstructor(ProjectionGeometry proj_geom, 
                                                     VolumeGeometry slice_geom,
                                                     VolumeGeometry preview_geom)
        : Reconstructor(slice_geom, preview_geom) {
    int num_angles = static_cast<int>(proj_geom.angles.size());
    int num_rows = proj_geom.row_count;
    int num_cols = proj_geom.col_count;
    float det_width = proj_geom.pixel_width;
    float det_height = proj_geom.pixel_height;
    auto angles = proj_geom.angles;

    bool vec_geometry = false;
    if (!vec_geometry) {
        auto proj_geom = astra::CParallelProjectionGeometry3D(
            num_angles, num_rows, num_cols, det_width, det_height, angles.data());

        proj_geom_slice_ = utils::proj_to_vec(&proj_geom);
        proj_geom_preview_ = utils::proj_to_vec(&proj_geom);

    } else {
        auto par_projs = utils::list_to_par_projections(angles);

        proj_geom_slice_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, par_projs.data());
        proj_geom_preview_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, par_projs.data());
    }

    spdlog::info("- Slice projection geometry: {}", details::astraInfo(*proj_geom_slice_));
    spdlog::info("- Preview projection geometry: {}", details::astraInfo(*proj_geom_preview_));

    vectors_ = std::vector<astra::SPar3DProjection>(
        proj_geom_slice_->getProjectionVectors(),
        proj_geom_slice_->getProjectionVectors() + num_angles);
    original_vectors_ = vectors_;
    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(num_angles * num_cols * num_rows, 0.0f);

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < 2; ++i) {
        gpu_mem_proj_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), num_cols, num_angles, num_rows));
        proj_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            proj_geom_slice_.get(), gpu_mem_proj_[0]));

        algo_slice_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_slice_.get()));
        algo_preview_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_preview_.get()));
    }
}

void ParallelBeamReconstructor::reconstructSlice(
        Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {

#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    auto k = vol_geom_slice_->getWindowMaxX();

    auto [delta, rot, scale] = utils::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    int num_angles = proj_geom_slice_->getProjectionCount();
    int num_rows = proj_geom_slice_->getDetectorRowCount();
    int num_cols = proj_geom_slice_->getDetectorColCount();
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

        vec_buf_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2],
                       px[0], px[1], px[2], py[0], py[1], py[2]};
        ++i;
    }

    proj_geom_slice_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        num_angles, num_rows, num_cols, vec_buf_.data());

    proj_data_[buffer_idx]->changeGeometry(proj_geom_slice_.get());
    algo_slice_[buffer_idx]->run();

    unsigned int n = vol_geom_slice_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), vol_mem_slice_, pos);

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Reconstructing slices "
                 "([{:.2f}, {:.2f}, {:.2f}], [{:.2f}, {:.2f}, {:.2f}], [{:.2f}, {:.2f}, {:.2f}])"
                 " from buffer {} took {} ms",
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx, duration / 1000);
#endif

}

void ParallelBeamReconstructor::reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) {

#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    proj_data_[buffer_idx]->changeGeometry(proj_geom_preview_.get());
    algo_preview_[buffer_idx]->run();

    unsigned int n = vol_geom_preview_->getGridRowCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), vol_mem_preview_, pos);

    float factor = n / (float)proj_geom_preview_->getDetectorColCount();
    // FIXME: why cubic? 
    float scale = factor * factor * factor;
    for (auto& x : buffer) x *= scale;

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Reconstructing preview took {} ms", duration / 1000);
#endif
}

// class ConeBeamReconstructor

ConeBeamReconstructor::ConeBeamReconstructor(ProjectionGeometry proj_geom, 
                                             VolumeGeometry slice_geom,
                                             VolumeGeometry preview_geom)
        : Reconstructor(slice_geom, preview_geom) {
    int num_angles = static_cast<int>(proj_geom.angles.size());
    int num_rows = proj_geom.row_count;
    int num_cols = proj_geom.col_count;
    float det_width = proj_geom.pixel_width;
    float det_height = proj_geom.pixel_height;
    auto angles = proj_geom.angles;
    float src2origin = proj_geom.source2origin;
    float origin2det = proj_geom.origin2detector;

    bool vec_geometry = false;
    if (!vec_geometry) {
        // TODO: should detector_size be (Height, Width)?
        auto proj_geom = astra::CConeProjectionGeometry3D(
            num_angles, num_rows, num_cols, det_width, det_height, angles.data(), src2origin, origin2det);

        proj_geom_slice_ = utils::proj_to_vec(&proj_geom);
        proj_geom_preview_ = utils::proj_to_vec(&proj_geom);
    } else {
        auto cone_projs = utils::list_to_cone_projections(num_rows, num_cols, angles);

        proj_geom_slice_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, cone_projs.data());
        proj_geom_preview_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, cone_projs.data());
    }
    
    spdlog::info("- Slice projection geometry: {}", details::astraInfo(*proj_geom_slice_));
    spdlog::info("- Preview projection geometry: {}", details::astraInfo(*proj_geom_preview_));

    vectors_ = std::vector<astra::SConeProjection>(
        proj_geom_slice_->getProjectionVectors(), proj_geom_slice_->getProjectionVectors() + num_angles);

    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(num_angles * num_cols * num_rows, 0.0f);

    // Projection data
    for (int i = 0; i < 2; ++i) {
        gpu_mem_proj_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), num_cols, num_angles, num_rows));
        proj_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            proj_geom_slice_.get(), gpu_mem_proj_[0]));
    }

    // Back projection algorithm, link to previously made objects
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < 2; ++i) {
        algo_slice_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_slice_.get()));
        algo_preview_.push_back(
            std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), proj_data_[i].get(), vol_data_preview_.get()));
    }
}

void ConeBeamReconstructor::reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {
    auto k = vol_geom_slice_->getWindowMaxX();

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

        vec_buf_[i] = {s[0],  s[1],  s[2],  d[0],  d[1],  d[2],
                       t1[0], t1[1], t1[2], t2[0], t2[1], t2[2]};
        ++i;
    }

    int num_angles = proj_geom_slice_->getProjectionCount();
    int num_rows = proj_geom_slice_->getDetectorRowCount();
    int num_cols = proj_geom_slice_->getDetectorColCount();
    proj_geom_slice_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
        num_angles, num_rows, num_cols, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    proj_data_[buffer_idx]->changeGeometry(proj_geom_slice_.get());
    algo_slice_[buffer_idx]->run();

    unsigned int n = vol_geom_slice_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), vol_mem_slice_, pos);
}

void ConeBeamReconstructor::reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) {
    proj_data_[buffer_idx]->changeGeometry(proj_geom_preview_.get());
    algo_preview_[buffer_idx]->run();

    unsigned int n = vol_geom_preview_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), vol_mem_preview_, pos);
}

std::vector<float> ConeBeamReconstructor::fdk_weights() {
    int num_angles = proj_geom_slice_->getProjectionCount();
    int num_rows = proj_geom_slice_->getDetectorRowCount();
    int num_cols = proj_geom_slice_->getDetectorColCount();
    auto result = std::vector<float>(num_angles * num_rows * num_cols, 0.0f);

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

} // tomcat::recon
