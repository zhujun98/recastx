#include <spdlog/spdlog.h>

#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"
#include "common/scoped_timer.hpp"

namespace recastx::recon {

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

// class Uploader

Uploader::Uploader(size_t group_size) : start_{0}, group_size_{group_size} {}

void Uploader::upload(astra::CFloat32ProjectionData3DGPU* dst, const float* src, size_t n) {

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Uploading sinograms to GPU");
#endif

    size_t start = start_;
    size_t end = (start + n - 1) % group_size_;
    spdlog::debug("Update sinograms {} - {}", start, end);

    if (end > start) {
        astra::uploadMultipleProjections(dst, src, start, end);
    } else {
        astra::uploadMultipleProjections(dst, src, start, group_size_ - 1);
        astra::uploadMultipleProjections(dst, src, 0, end);
    }

    start_ = (end + 1) % group_size_;
    spdlog::info("Finished uploading sinograms to GPU");
}

// class Reconstructor

Reconstructor::Reconstructor(const ProjectionGeometry& p_geom, 
                             const VolumeGeometry& s_geom, 
                             const VolumeGeometry& v_geom,
                             bool double_buffer) {
    s_geom_ = makeVolumeGeometry(s_geom);
    s_mem_ = makeVolumeGeometryMemHandle(s_geom);
    s_data_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(s_geom_.get(), s_mem_);
    spdlog::info("- Slice volume geometry: {}", details::astraInfo(*s_geom_));
    
    v_geom_ = makeVolumeGeometry(v_geom);
    v_mem_ = makeVolumeGeometryMemHandle(v_geom);
    v_data_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(v_geom_.get(), v_mem_);
    spdlog::info("- Preview volume geometry: {}", details::astraInfo(*v_geom_));

    for (int i = 0; i < (double_buffer ? 2 : 1); ++i) {
        uploader_.emplace_back(p_geom.angles.size());
    }
}

Reconstructor::~Reconstructor() {
    spdlog::info("Deconstructing Reconstructor and freeing GPU memory ...");

    astraCUDA3d::freeGPUMemory(s_mem_);
    astraCUDA3d::freeGPUMemory(v_mem_);
    for (auto& handle : p_mem_) astraCUDA3d::freeGPUMemory(handle);
}

void Reconstructor::uploadSinograms(int buffer_idx, const float* data, size_t n) {
    uploader_[buffer_idx].upload(p_data_[buffer_idx].get(), data, n);
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

ParallelBeamReconstructor::ParallelBeamReconstructor(size_t col_count, 
                                                     size_t row_count,
                                                     const ProjectionGeometry& p_geom, 
                                                     const VolumeGeometry& s_geom,
                                                     const VolumeGeometry& v_geom,
                                                     bool double_buffer)
        : Reconstructor(p_geom, s_geom, v_geom, double_buffer) {

    auto& angles = p_geom.angles;
    size_t angle_count = angles.size();

    bool vec_geometry = false;
    if (!vec_geometry) {
        auto geom = astra::CParallelProjectionGeometry3D(
            angle_count, row_count, col_count, p_geom.pixel_width, p_geom.pixel_height, angles.data());

        s_p_geom_ = utils::proj_to_vec(&geom);
        v_p_geom_ = utils::proj_to_vec(&geom);

    } else {
        auto par_projs = utils::list_to_par_projections(angles);

        s_p_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            angle_count, row_count, col_count, par_projs.data());
        v_p_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            angle_count, row_count, col_count, par_projs.data());
    }

    spdlog::info("- Slice projection geometry: {}", details::astraInfo(*s_p_geom_));
    spdlog::info("- Preview projection geometry: {}", details::astraInfo(*v_p_geom_));

    vectors_ = std::vector<astra::SPar3DProjection>(
        s_p_geom_->getProjectionVectors(), s_p_geom_->getProjectionVectors() + angle_count);
    original_vectors_ = vectors_;
    vec_buf_ = vectors_;

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    auto buf = std::vector<float>(col_count * angle_count * row_count, 0.0f);
    for (int i = 0; i < (double_buffer ? 2 : 1); ++i) {
        p_mem_.push_back(astraCUDA3d::createProjectionArrayHandle(
            buf.data(), col_count, angle_count, row_count));
        p_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            s_p_geom_.get(), p_mem_[0]));

        s_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), p_data_[i].get(), s_data_.get()));
        v_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), p_data_[i].get(), v_data_.get()));
    }
}

void ParallelBeamReconstructor::reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Reconstructing slice");
#endif

    auto k = s_geom_->getWindowMaxX();

    auto [delta, rot, scale] = utils::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    int angle_count = s_p_geom_->getProjectionCount();
    int num_rows = s_p_geom_->getDetectorRowCount();
    int num_cols = s_p_geom_->getDetectorColCount();
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

    s_p_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        angle_count, num_rows, num_cols, vec_buf_.data());

    spdlog::debug("Reconstructing slice with buffer index: {}", buffer_idx);
    p_data_[buffer_idx]->changeGeometry(s_p_geom_.get());
    s_algo_[buffer_idx]->run();

    unsigned int n = s_geom_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), s_mem_, pos);
}

void ParallelBeamReconstructor::reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) {

#if (VERBOSITY >= 2)
    ScopedTimer timer("Bench", "Reconstructing preview");
#endif

    spdlog::debug("Reconstructing preview with buffer index: {}", buffer_idx);
    p_data_[buffer_idx]->changeGeometry(v_p_geom_.get());
    v_algo_[buffer_idx]->run();

    unsigned int n = v_geom_->getGridRowCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), v_mem_, pos);

    float factor = n / (float)v_p_geom_->getDetectorColCount();
    // FIXME: why cubic? 
    float scale = factor * factor * factor;
    for (auto& x : buffer) x *= scale;
}

// class ConeBeamReconstructor

ConeBeamReconstructor::ConeBeamReconstructor(size_t col_count, 
                                             size_t row_count,
                                             const ProjectionGeometry& p_geom, 
                                             const VolumeGeometry& s_geom,
                                             const VolumeGeometry& v_geom,
                                             bool double_buffer)
        : Reconstructor(p_geom, s_geom, v_geom, double_buffer) {

    auto& angles = p_geom.angles;
    size_t angle_count = angles.size();

    bool vec_geometry = false;
    if (!vec_geometry) {
        // TODO: should detector_size be (Height, Width)?
        auto geom = astra::CConeProjectionGeometry3D(
            angle_count, row_count, col_count, 
            p_geom.pixel_width, p_geom.pixel_height, angles.data(), 
            p_geom.source2origin, p_geom.origin2detector);

        s_p_geom_ = utils::proj_to_vec(&geom);
        v_p_geom_ = utils::proj_to_vec(&geom);
    } else {
        auto cone_projs = utils::list_to_cone_projections(row_count, col_count, angles);

        s_p_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            angle_count, row_count, col_count, cone_projs.data());
        v_p_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            angle_count, row_count, col_count, cone_projs.data());
    }
    
    spdlog::info("- Slice projection geometry: {}", details::astraInfo(*s_p_geom_));
    spdlog::info("- Preview projection geometry: {}", details::astraInfo(*v_p_geom_));

    vectors_ = std::vector<astra::SConeProjection>(
        s_p_geom_->getProjectionVectors(), s_p_geom_->getProjectionVectors() + angle_count);

    vec_buf_ = vectors_;

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    auto buf = std::vector<float>(col_count * angle_count * row_count, 0.0f);
    for (int i = 0; i < (double_buffer ? 2 : 1); ++i) {
        p_mem_.push_back(astraCUDA3d::createProjectionArrayHandle(
            buf.data(), col_count, angle_count, row_count));
        p_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            s_p_geom_.get(), p_mem_[0]));

        s_algo_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), p_data_[i].get(), s_data_.get()));
        v_algo_.push_back(
            std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), p_data_[i].get(), v_data_.get()));
    }
}

void ConeBeamReconstructor::reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) {
    auto k = s_geom_->getWindowMaxX();

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

    int angle_count = s_p_geom_->getProjectionCount();
    int num_rows = s_p_geom_->getDetectorRowCount();
    int num_cols = s_p_geom_->getDetectorColCount();
    s_p_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
        angle_count, num_rows, num_cols, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    p_data_[buffer_idx]->changeGeometry(s_p_geom_.get());
    s_algo_[buffer_idx]->run();

    unsigned int n = s_geom_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), s_mem_, pos);
}

void ConeBeamReconstructor::reconstructPreview(int buffer_idx, Tensor<float, 3>& buffer) {
    p_data_[buffer_idx]->changeGeometry(v_p_geom_.get());
    v_algo_[buffer_idx]->run();

    unsigned int n = v_geom_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(buffer.data(), v_mem_, pos);
}

std::vector<float> ConeBeamReconstructor::fdk_weights() {
    int angle_count = s_p_geom_->getProjectionCount();
    int num_rows = s_p_geom_->getDetectorRowCount();
    int num_cols = s_p_geom_->getDetectorColCount();
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

} // namespace recastx::recon
