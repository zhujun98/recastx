#include <spdlog/spdlog.h>

#include "recon/reconstructor.hpp"
#include "recon/utils.hpp"

namespace tomcat::recon {

// class Reconstructor

Reconstructor::Reconstructor(VolumeGeometry slice_geom, VolumeGeometry preview_geom) {
    vol_geom_slice_ = std::move(Reconstructor::makeVolumeGeometry(slice_geom));
    spdlog::info("astra: slice geometry initialized: {}", utils::info(*vol_geom_slice_));

    gpu_mem_slice_ = Reconstructor::allocateGpuMemory(slice_geom);

    vol_data_slice_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(vol_geom_slice_.get(), gpu_mem_slice_);
    
    spdlog::info("astra: slice data initialized");

    // Small preview volume
    vol_geom_preview_ = std::move(Reconstructor::makeVolumeGeometry(preview_geom));
    spdlog::info("astra: preview geometry initialized: {}", utils::info(*vol_geom_preview_));

    gpu_mem_preview_ = Reconstructor::allocateGpuMemory(preview_geom);

    vol_data_preview_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
        vol_geom_preview_.get(), gpu_mem_preview_);

    spdlog::info("astra: preview data initialized");
}

Reconstructor::~Reconstructor() {
    spdlog::info("Deconstructing Reconstructor and freeing GPU memory ...");

    astraCUDA3d::freeGPUMemory(gpu_mem_slice_);
    astraCUDA3d::freeGPUMemory(gpu_mem_preview_);
    for (auto& handle : gpu_mem_proj_) astraCUDA3d::freeGPUMemory(handle);
}

void Reconstructor::uploadSinograms(int buffer_idx, 
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

std::unique_ptr<astra::CVolumeGeometry3D> Reconstructor::makeVolumeGeometry(const VolumeGeometry& geom) {
    return std::make_unique<astra::CVolumeGeometry3D>(
        geom.col_count, geom.row_count, geom.slice_count,
        geom.min_x, geom.min_y, geom.min_z,
        geom.max_x, geom.max_y, geom.max_z);
}

astraCUDA3d::MemHandle3D Reconstructor::allocateGpuMemory(const VolumeGeometry& geom) {
    return astraCUDA3d::allocateGPUMemory(
        geom.col_count, geom.row_count, geom.slice_count, astraCUDA3d::INIT_ZERO);
}

// class ParallelBeamReconstructor

ParallelBeamReconstructor::ParallelBeamReconstructor(ProjectionGeometry proj_geom, 
                                                     VolumeGeometry slice_geom,
                                                     VolumeGeometry preview_geom)
        : Reconstructor(slice_geom, preview_geom) {
    spdlog::info("Initializing parallel beam reconstructor ...");

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

void ParallelBeamReconstructor::reconstructSlice(std::vector<float>& slice_buffer, 
                                                 Orientation x, 
                                                 int buffer_idx) {

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
    astraCUDA3d::copyFromGPUMemory(slice_buffer.data(), gpu_mem_slice_, pos);

#if (VERBOSITY >= 2)
    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() -  start).count();
    spdlog::info("[bench] Reconstructing slices "
                 "([{:.2f}, {:.2f}, {:.2f}], [{:.2f}, {:.2f}, {:.2f}], [{:.2f}, {:.2f}, {:.2f}])"
                 " from buffer {} took {} ms",
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx, duration / 1000);
#endif

}

void ParallelBeamReconstructor::reconstructPreview(std::vector<float>& preview_buffer, 
                                                   int buffer_idx) {
#if (VERBOSITY >= 2)
    auto start = std::chrono::steady_clock::now();
#endif

    proj_data_[buffer_idx]->changeGeometry(proj_geom_preview_.get());
    algo_preview_[buffer_idx]->run();

    unsigned int n = vol_geom_preview_->getGridRowCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), gpu_mem_preview_, pos);

    float factor = n / (float)proj_geom_preview_->getDetectorColCount();
    // FIXME: why cubic? 
    float scale = factor * factor * factor;
    for (auto& x : preview_buffer) x *= scale;

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
    spdlog::info("Initializing cone beam solver ...");

    int num_angles = static_cast<int>(proj_geom.angles.size());
    int num_rows = proj_geom.row_count;
    int num_cols = proj_geom.col_count;
    float det_width = proj_geom.pixel_width;
    float det_height = proj_geom.pixel_height;
    auto angles = proj_geom.angles;
    float source2origin = proj_geom.source2origin;
    float origin2det = proj_geom.origin2detector;

    bool vec_geometry = false;
    if (!vec_geometry) {
        // TODO: should detector_size be (Height, Width)?
        auto proj_geom = astra::CConeProjectionGeometry3D(
            num_angles, num_rows, num_cols, det_width, det_height, angles.data(), source2origin, origin2det);

        proj_geom_slice_ = utils::proj_to_vec(&proj_geom);
        proj_geom_preview_ = utils::proj_to_vec(&proj_geom);
    } else {
        auto cone_projs = utils::list_to_cone_projections(num_rows, num_cols, angles);
        proj_geom_slice_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, cone_projs.data());

        spdlog::info("{}", utils::info(*proj_geom_slice_));

        proj_geom_preview_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            num_angles, num_rows, num_cols, cone_projs.data());
    }

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

void ConeBeamReconstructor::reconstructSlice(std::vector<float>& slice_buffer, 
                                             Orientation x, 
                                             int buffer_idx) {
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
    astraCUDA3d::copyFromGPUMemory(slice_buffer.data(), gpu_mem_slice_, pos);
}

void ConeBeamReconstructor::reconstructPreview(std::vector<float>& preview_buffer, int buffer_idx) {
    proj_data_[buffer_idx]->changeGeometry(proj_geom_preview_.get());
    algo_preview_[buffer_idx]->run();

    unsigned int n = vol_geom_preview_->getGridColCount();
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), gpu_mem_preview_, pos);
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

std::unique_ptr<Reconstructor> createReconstructor(
        bool cone_beam, int num_rows, int num_cols, int num_angles, 
        float pixel_h, float pixel_w, float source2origin, float origin2det,
        int slice_size, int preview_size, 
        std::array<float, 3> vol_mins, std::array<float, 3> vol_maxs) {

    ProjectionGeometry proj_geom = {
        .col_count = num_cols,
        .row_count = num_rows,
        .pixel_width = pixel_w,
        .pixel_height = pixel_h,
        // TODO:: receive/create angles in different ways.
        .angles = utils::defaultAngles(num_angles),
        .source2origin = source2origin,
        .origin2detector = origin2det
    };

    float half_slab_height = 0.5f * (vol_maxs[2] - vol_mins[2]) / preview_size;
    float z0 = 0.5f * (vol_maxs[2] + vol_mins[2]);

    VolumeGeometry slice_geom = {
        .col_count = slice_size,
        .row_count = slice_size,
        .slice_count = 1,
        .min_x = vol_mins[0],
        .min_y = vol_mins[1],
        .min_z = z0 - half_slab_height,
        .max_x = vol_maxs[0],
        .max_y = vol_maxs[1],
        .max_z = z0 + half_slab_height
    };
    
    VolumeGeometry preview_geom = {
        .col_count = preview_size,
        .row_count = preview_size,
        .slice_count = preview_size,
        .min_x = vol_mins[0],
        .min_y = vol_mins[1],
        .min_z = vol_mins[2],
        .max_x = vol_maxs[0],
        .max_y = vol_maxs[1],
        .max_z = vol_maxs[2]
    };

    if (cone_beam)
        return std::make_unique<ConeBeamReconstructor>(proj_geom, slice_geom, preview_geom);
    return std::make_unique<ParallelBeamReconstructor>(proj_geom, slice_geom, preview_geom);
}

} // tomcat::recon
