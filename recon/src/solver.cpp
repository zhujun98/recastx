#include <spdlog/spdlog.h>

#include "recon/solver.hpp"
#include "recon/utils.hpp"

namespace tomcat::recon {

// class Solver

Solver::Solver(int rows, 
               int cols, 
               int projections,
               const std::array<float, 3>& volume_min_point, 
               const std::array<float, 3>& volume_max_point,
               int preview_size,
               int slice_size)
        : rows_(rows),
          cols_(cols),
          projections_(projections),
          preview_size_(preview_size),
          slice_size_(slice_size) {

    float half_slab_height = 0.5f * (volume_max_point[2] - volume_min_point[2]) / preview_size;
    float mid_z = 0.5f * (volume_max_point[2] + volume_min_point[2]);

    // This class represents a 3D pixel grid that is placed in the geometry. 
    // It defines a rectangular volume window.
    vol_geom_ = std::make_unique<astra::CVolumeGeometry3D>(
        slice_size, slice_size, 1,
        volume_min_point[0], volume_min_point[1], mid_z - half_slab_height, 
        volume_max_point[0], volume_max_point[1], mid_z + half_slab_height);

    spdlog::info("Slice volume: {}", utils::info(*vol_geom_));

    // Volume data
    vol_handle_ = astraCUDA3d::allocateGPUMemory(slice_size, slice_size, 1, astraCUDA3d::INIT_ZERO);
    vol_data_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(vol_geom_.get(), vol_handle_);

    // Small preview volume
    vol_geom_small_ = std::make_unique<astra::CVolumeGeometry3D>(
        preview_size, preview_size, preview_size, 
        volume_min_point[0], volume_min_point[1], volume_min_point[2],
        volume_max_point[0], volume_max_point[1], volume_max_point[2]);

    spdlog::info("Small preview volume: {}", utils::info(*vol_geom_small_));

    vol_handle_small_ = astraCUDA3d::allocateGPUMemory(
        preview_size, preview_size, preview_size, astraCUDA3d::INIT_ZERO);
    vol_data_small_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
        vol_geom_small_.get(), vol_handle_small_);
}

Solver::~Solver() {
    spdlog::info("Deconstructing solver and freeing GPU memory ...");

    astraCUDA3d::freeGPUMemory(vol_handle_);
    astraCUDA3d::freeGPUMemory(vol_handle_small_);
    for (auto& proj_handle : proj_handles_) {
        astraCUDA3d::freeGPUMemory(proj_handle);
    }
}

void Solver::uploadSinograms(int buffer_idx, 
                             const std::vector<float>& sino, 
                             int begin, 
                             int end) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    astra::uploadMultipleProjections(proj_data_[buffer_idx].get(), &sino[0], begin, end);

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Uploading sinograms to GPU took {} ms", duration / 1000);
#endif
}

// class ParallelBeamSolver

ParallelBeamSolver::ParallelBeamSolver(int rows, 
                                       int cols,
                                       std::vector<float> angles,
                                       const std::array<float, 3>& volume_min_point, 
                                       const std::array<float, 3>& volume_max_point,
                                       int preview_size,
                                       int slice_size,
                                       bool vec_geometry,
                                       const std::array<float, 2>& detector_size)
        : Solver(rows, 
                 cols, 
                 static_cast<int>(angles.size()), 
                 volume_min_point, 
                 volume_max_point, 
                 preview_size, 
                 slice_size) {
    spdlog::info("Initializing parallel beam solver ...");
    if (!vec_geometry) {
        auto proj_geom = astra::CParallelProjectionGeometry3D(
            projections_, rows, cols, detector_size[1], detector_size[0], angles.data());

        proj_geom_ = utils::proj_to_vec(&proj_geom);

        proj_geom_small_ = utils::proj_to_vec(&proj_geom);

    } else {
        auto par_projs = utils::list_to_par_projections(angles);

        proj_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            projections_, rows, cols, par_projs.data());

        proj_geom_small_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            projections_, rows, cols, par_projs.data());
    }

    vectors_ = std::vector<astra::SPar3DProjection>(
        proj_geom_->getProjectionVectors(),
        proj_geom_->getProjectionVectors() + projections_);
    original_vectors_ = vectors_;
    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(projections_ * cols * rows, 0.0f);

    // Projection data and back projection algorithm
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < 2; ++i) {
        proj_handles_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), cols, projections_, rows));
        proj_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            proj_geom_.get(), proj_handles_[0]));

        algs_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_.get()));
        algs_small_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_small_.get()));
    }
}

void ParallelBeamSolver::reconstructSlice(std::vector<float>& slice_buffer, 
                                          Orientation x, 
                                          int buffer_idx) {

#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    auto k = vol_geom_->getWindowMaxX();

    auto [delta, rot, scale] = utils::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] : vectors_) {
        auto r = Eigen::Vector3f(rx, ry, rz);
        auto d = Eigen::Vector3f(dx, dy, dz);
        auto px = Eigen::Vector3f(pxx, pxy, pxz);
        auto py = Eigen::Vector3f(pyx, pyy, pyz);

        d += 0.5f * (cols_ * px + rows_ * py);
        r = scale.cwiseProduct(rot * r);
        d = scale.cwiseProduct(rot * (d + delta));
        px = scale.cwiseProduct(rot * px);
        py = scale.cwiseProduct(rot * py);
        d -= 0.5f * (cols_ * px + rows_ * py);

        vec_buf_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2],
                       px[0], px[1], px[2], py[0], py[1], py[2]};
        ++i;
    }

    proj_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        projections_, rows_, cols_, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    proj_data_[buffer_idx]->changeGeometry(proj_geom_.get());
    algs_[buffer_idx]->run();

    unsigned int n = slice_size_;
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(slice_buffer.data(), vol_handle_, pos);

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Reconstructing slices took {} ms", duration / 1000);
#endif

}

void ParallelBeamSolver::reconstructPreview(std::vector<float>& preview_buffer, 
                                            int buffer_idx) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    proj_data_[buffer_idx]->changeGeometry(proj_geom_small_.get());
    algs_small_[buffer_idx]->run();

    unsigned int n = preview_size_;
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), vol_handle_small_, pos);

    float factor = n / (float)cols_;
    // FIXME: why cubic? 
    float scale = factor * factor * factor;
    for (auto& x : preview_buffer) x *= scale;

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Reconstructing preview took {} ms", duration / 1000);
#endif
}

bool ParallelBeamSolver::parameterChanged(std::string param, std::variant<float, std::string, bool> value) {

    bool tilt_changed = false;
    if (param == "tilt angle") {
        tilt_changed = true;
        tilt_rotate_ = std::get<float>(value);
        // TODO rotate geometry vectors
    } else if (param == "tilt translate") {
        tilt_changed = true;
        tilt_translate_ = std::get<float>(value);
        // TODO translate geometry vectors
    }

    spdlog::info("Rotate to {}, translate to {}.", tilt_rotate_, tilt_translate_);

    if (tilt_changed) {
        // From the ASTRA geometry, get the vectors, modify, and reset them
        int i = 0;
        for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] :
             original_vectors_) {
            auto r = Eigen::Vector3f(rx, ry, rz);
            auto d = Eigen::Vector3f(dx, dy, dz);
            auto px = Eigen::Vector3f(pxx, pxy, pxz);
            auto py = Eigen::Vector3f(pyx, pyy, pyz);

            d += tilt_translate_ * px;

            auto z = px.normalized();
            auto w = py.normalized();
            auto axis = z.cross(w);
            auto rot = Eigen::AngleAxis<float>(tilt_rotate_ * M_PI / 180.0f, axis.normalized()).matrix();

            px = rot * px;
            py = rot * py;

            vectors_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2],
                           px[0], px[1], px[2], py[0], py[1], py[2]};
            ++i;
        }

        // TODO if either changed, trigger a new reconstruction. Do we need to
        // do this from reconstructor (since we don't have access to listeners
        // to notify?). Then the order of handling parameters matters.
    }

    return tilt_changed;
}

// class ConeBeamSolver

ConeBeamSolver::ConeBeamSolver(int rows, 
                               int cols,
                               std::vector<float> angles,
                               const std::array<float, 3>& volume_min_point, 
                               const std::array<float, 3>& volume_max_point,
                               int preview_size,
                               int slice_size,
                               bool vec_geometry,
                               const std::array<float, 2>& detector_size,
                               float source_origin,
                               float origin_det)
        : Solver(rows, 
                 cols, 
                 static_cast<int>(angles.size()), 
                 volume_min_point, 
                 volume_max_point, 
                 preview_size, 
                 slice_size) {
    spdlog::info("Initializing cone beam solver ...");

    if (!vec_geometry) {
        // TODO: should detector_size be (Height, Width)?
        auto proj_geom = astra::CConeProjectionGeometry3D(
            projections_, rows, cols,
            detector_size[1], detector_size[0],
            angles.data(), source_origin, origin_det);

        proj_geom_ = utils::proj_to_vec(&proj_geom);
        proj_geom_small_ = utils::proj_to_vec(&proj_geom);
    } else {
        auto cone_projs = utils::list_to_cone_projections(rows, cols, angles);
        proj_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            projections_, rows, cols, cone_projs.data());

        spdlog::info("{}", utils::info(*proj_geom_));

        proj_geom_small_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            projections_, rows, cols, cone_projs.data());
    }

    vectors_ = std::vector<astra::SConeProjection>(
        proj_geom_->getProjectionVectors(), proj_geom_->getProjectionVectors() + projections_);

    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(projections_ * cols * rows, 0.0f);

    // Projection data
    for (int i = 0; i < 2; ++i) {
        proj_handles_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), cols, projections_, rows));
        proj_data_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            proj_geom_.get(), proj_handles_[0]));
    }

    // Back projection algorithm, link to previously made objects
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < 2; ++i) {
        algs_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_data_[i].get(), vol_data_.get()));
        algs_small_.push_back(
            std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), proj_data_[i].get(), vol_data_small_.get()));
    }
}

void ConeBeamSolver::reconstructSlice(std::vector<float>& slice_buffer, 
                                      Orientation x, 
                                      int buffer_idx) {
    auto k = vol_geom_->getWindowMaxX();

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

    proj_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
        projections_, rows_, cols_, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    proj_data_[buffer_idx]->changeGeometry(proj_geom_.get());
    algs_[buffer_idx]->run();

    unsigned int n = slice_size_;
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(slice_buffer.data(), vol_handle_, pos);
}

void ConeBeamSolver::reconstructPreview(std::vector<float>& preview_buffer, int buffer_idx) {
    proj_data_[buffer_idx]->changeGeometry(proj_geom_small_.get());
    algs_small_[buffer_idx]->run();

    unsigned int n = preview_size_;
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), vol_handle_small_, pos);
}

std::vector<float> ConeBeamSolver::fdk_weights() {
    auto result = std::vector<float>(cols_ * rows_ * projections_, 0.0f);

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
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                auto y = d + r * t2 + c * t1;
                auto denum = (y - s).norm();
                result[(i * cols_ * rows_) + (r * cols_) + c] = rho / denum;
            }
        }

        ++i;
    }

    return result;
}

} // tomcat::recon
