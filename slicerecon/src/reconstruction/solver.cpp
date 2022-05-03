#include <spdlog/spdlog.h>

#include "slicerecon/reconstruction/solver.hpp"
#include "slicerecon/util/bench.hpp"
#include "slicerecon/util/util.hpp"

namespace slicerecon {

Solver::Solver(Settings param, Geometry geom) : param_(param), geom_(geom) {
    float half_slab_height =
        0.5f * (geom_.volume_max_point[2] - geom_.volume_min_point[2]) / param_.preview_size;
    float mid_z = 
        0.5f * (geom_.volume_max_point[2] + geom_.volume_min_point[2]);

    // Volume geometry
    vol_geom_ = std::make_unique<astra::CVolumeGeometry3D>(
        param_.slice_size, param_.slice_size, 1,
        geom_.volume_min_point[0], geom_.volume_min_point[1],
        mid_z - half_slab_height, geom_.volume_max_point[0],
        geom_.volume_max_point[1], mid_z + half_slab_height);

    spdlog::info("Slice volume: {}", slicerecon::util::info(*vol_geom_));

    // Volume data
    vol_handle_ = astraCUDA3d::allocateGPUMemory(param_.slice_size,
                                                 param_.slice_size, 1,
                                                 astraCUDA3d::INIT_ZERO);
    vol_data_ = std::make_unique<astra::CFloat32VolumeData3DGPU>(
        vol_geom_.get(), vol_handle_);

    // Small preview volume
    vol_geom_small_ = std::make_unique<astra::CVolumeGeometry3D>(
        param_.preview_size, param_.preview_size,
        param_.preview_size, geom_.volume_min_point[0],
        geom_.volume_min_point[1], geom_.volume_min_point[1],
        geom_.volume_max_point[0], geom_.volume_max_point[1],
        geom_.volume_max_point[1]);

    spdlog::info("Small preview volume: {}", slicerecon::util::info(*vol_geom_small_));

    vol_handle_small_ = astraCUDA3d::allocateGPUMemory(
        param_.preview_size, param_.preview_size,
        param_.preview_size, astraCUDA3d::INIT_ZERO);
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

ParallelBeamSolver::ParallelBeamSolver(Settings param, Geometry geom)
    : Solver(param, geom) {
    spdlog::info("Initializing parallel beam solver ...");
    if (!geom_.vec_geometry) {
        // Projection geometry
        auto proj_geom = astra::CParallelProjectionGeometry3D(
            geom_.proj_count, geom_.rows, geom_.cols, 1.0f, 1.0f, geom_.angles.data());

        proj_geom_ = slicerecon::util::proj_to_vec(&proj_geom);

        proj_geom_small_ = slicerecon::util::proj_to_vec(&proj_geom);

    } else {
        auto par_projs = slicerecon::util::list_to_par_projections(geom_.angles);
        proj_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
            geom_.proj_count, geom_.rows, geom_.cols, par_projs.data());
        proj_geom_small_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
                geom_.proj_count, geom_.rows, geom_.cols, par_projs.data());
    }

    vectors_ = std::vector<astra::SPar3DProjection>(
        proj_geom_->getProjectionVectors(),
        proj_geom_->getProjectionVectors() + geom_.proj_count);
    original_vectors_ = vectors_;
    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(geom_.proj_count * geom_.cols * geom_.rows, 0.0f);

    // Projection data
    int nr_handles = param.reconstruction_mode == Mode::alternating ? 2 : 1;
    for (int i = 0; i < nr_handles; ++i) {
        proj_handles_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), geom_.cols, geom_.proj_count, geom_.rows));
        proj_datas_.push_back(std::make_unique<astra::CFloat32ProjectionData3DGPU>(
            proj_geom_.get(), proj_handles_[0]));
    }

    // Back projection algorithm, link to previously made objects
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < nr_handles; ++i) {
        algs_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_datas_[i].get(), vol_data_.get()));
        algs_small_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_datas_[i].get(), vol_data_small_.get()));
    }
}

slice_data ParallelBeamSolver::reconstruct_slice(orientation x, int buffer_idx) {
    auto dt = util::bench_scope("slice");
    auto k = vol_geom_->getWindowMaxX();

    auto [delta, rot, scale] = util::slice_transform(
        {x[6], x[7], x[8]}, {x[0], x[1], x[2]}, {x[3], x[4], x[5]}, k);

    // From the ASTRA geometry, get the vectors, modify, and reset them
    int i = 0;
    for (auto [rx, ry, rz, dx, dy, dz, pxx, pxy, pxz, pyx, pyy, pyz] : vectors_) {
        auto r = Eigen::Vector3f(rx, ry, rz);
        auto d = Eigen::Vector3f(dx, dy, dz);
        auto px = Eigen::Vector3f(pxx, pxy, pxz);
        auto py = Eigen::Vector3f(pyx, pyy, pyz);

        d += 0.5f * (geom_.cols * px + geom_.rows * py);
        r = scale.cwiseProduct(rot * r);
        d = scale.cwiseProduct(rot * (d + delta));
        px = scale.cwiseProduct(rot * px);
        py = scale.cwiseProduct(rot * py);
        d -= 0.5f * (geom_.cols * px + geom_.rows * py);

        vec_buf_[i] = {r[0],  r[1],  r[2],  d[0],  d[1],  d[2],
                       px[0], px[1], px[2], py[0], py[1], py[2]};
        ++i;
    }

    proj_geom_ = std::make_unique<astra::CParallelVecProjectionGeometry3D>(
        geom_.proj_count, geom_.rows, geom_.cols, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    proj_datas_[buffer_idx]->changeGeometry(proj_geom_.get());
    algs_[buffer_idx]->run();

    unsigned int n = param_.slice_size;
    auto result = std::vector<float>(n * n, 0.0f);
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(result.data(), vol_handle_, pos);

    return {{(int)n, (int)n}, std::move(result)};
}

void ParallelBeamSolver::reconstruct_preview(
    std::vector<float>& preview_buffer, int buffer_idx) {
    auto dt = util::bench_scope("3D preview");

    proj_datas_[buffer_idx]->changeGeometry(proj_geom_small_.get());
    algs_small_[buffer_idx]->run();

    unsigned int n = param_.preview_size;
    float factor = (n / (float)geom_.cols);
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), vol_handle_small_, pos);

    for (auto& x : preview_buffer) {
        x *= (factor * factor * factor);
    }
}

bool ParallelBeamSolver::parameter_changed(std::string param, std::variant<float, std::string, bool> value) {

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

std::vector<std::pair<std::string, std::variant<float, std::vector<std::string>, bool>>>
ParallelBeamSolver::parameters() {
    if (!param_.tilt_axis) {
        return {};
    }
    return {{"tilt angle", 0.0f}, {"tilt translate", 0.0f}};
}

ConeBeamSolver::ConeBeamSolver(Settings param, Geometry geom)
    : Solver(param, geom) {
    spdlog::info("Initializing cone beam solver ...");

    if (!geom_.vec_geometry) {
        auto proj_geom = astra::CConeProjectionGeometry3D(
            geom_.proj_count, geom_.rows, geom_.cols,
            geom_.detector_size[0], geom_.detector_size[1],
            geom_.angles.data(), geom_.source_origin, geom_.origin_det);

        proj_geom_ = slicerecon::util::proj_to_vec(&proj_geom);
        proj_geom_small_ = slicerecon::util::proj_to_vec(&proj_geom);
    } else {
        auto cone_projs = slicerecon::util::list_to_cone_projections(
            geom_.rows, geom_.cols, geom_.angles);
        proj_geom_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            geom_.proj_count, geom_.rows, geom_.cols, cone_projs.data());

        spdlog::info("{}", slicerecon::util::info(*proj_geom_));

        proj_geom_small_ = std::make_unique<astra::CConeVecProjectionGeometry3D>(
            geom_.proj_count, geom_.rows, geom_.cols, cone_projs.data());
    }

    vectors_ = std::vector<astra::SConeProjection>(
        proj_geom_->getProjectionVectors(),
        proj_geom_->getProjectionVectors() + geom_.proj_count);
    vec_buf_ = vectors_;

    auto zeros = std::vector<float>(geom_.proj_count * geom_.cols * geom_.rows, 0.0f);

    // Projection data
    int nr_handles = param.reconstruction_mode == Mode::alternating ? 2 : 1;
    for (int i = 0; i < nr_handles; ++i) {
        proj_handles_.push_back(astraCUDA3d::createProjectionArrayHandle(
            zeros.data(), geom_.cols, geom_.proj_count, geom_.rows));
        proj_datas_.push_back(
            std::make_unique<astra::CFloat32ProjectionData3DGPU>(
                proj_geom_.get(), proj_handles_[0]));
    }

    // Back projection algorithm, link to previously made objects
    projector_ = std::make_unique<astra::CCudaProjector3D>();
    for (int i = 0; i < nr_handles; ++i) {
        algs_.push_back(std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
            projector_.get(), proj_datas_[i].get(), vol_data_.get()));
        algs_small_.push_back(
            std::make_unique<astra::CCudaBackProjectionAlgorithm3D>(
                projector_.get(), proj_datas_[i].get(), vol_data_small_.get()));
    }
}

slice_data ConeBeamSolver::reconstruct_slice(orientation x, int buffer_idx) {
    auto dt = util::bench_scope("slice");
    auto k = vol_geom_->getWindowMaxX();

    auto [delta, rot, scale] = util::slice_transform(
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
        geom_.proj_count, geom_.rows, geom_.cols, vec_buf_.data());

    spdlog::info("Reconstructing slice: [{}, {}, {}], [{}, {}, {}], [{}, {}, {}] buffer ({}).", 
                 x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], buffer_idx);

    proj_datas_[buffer_idx]->changeGeometry(proj_geom_.get());
    algs_[buffer_idx]->run();

    unsigned int n = param_.slice_size;
    auto result = std::vector<float>(n * n, 0.0f);
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, 1, n, n, n, 1, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(result.data(), vol_handle_, pos);

    return {{(int)n, (int)n}, std::move(result)};
}

void ConeBeamSolver::reconstruct_preview(std::vector<float>& preview_buffer, int buffer_idx) {
    auto dt = util::bench_scope("3D preview");

    proj_datas_[buffer_idx]->changeGeometry(proj_geom_small_.get());
    algs_small_[buffer_idx]->run();

    unsigned int n = param_.preview_size;
    auto pos = astraCUDA3d::SSubDimensions3D{n, n, n, n, n, n, n, 0, 0, 0};
    astraCUDA3d::copyFromGPUMemory(preview_buffer.data(), vol_handle_small_, pos);
}

std::vector<float> ConeBeamSolver::fdk_weights() {
    auto result = std::vector<float>(geom_.cols * geom_.rows * geom_.proj_count, 0.0f);

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
        for (int r = 0; r < geom_.rows; ++r) {
            for (int c = 0; c < geom_.cols; ++c) {
                auto y = d + r * t2 + c * t1;
                auto denum = (y - s).norm();
                result[(i * geom_.cols * geom_.rows) + (r * geom_.cols) + c] = rho / denum;
            }
        }

        ++i;
    }

    return result;
}

} // slicerecon