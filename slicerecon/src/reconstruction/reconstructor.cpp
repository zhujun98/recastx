#include <complex>
#include <numeric>

#include <Eigen/Eigen>
#include <spdlog/spdlog.h>

#include "slicerecon/reconstruction/reconstructor.hpp"
#include "slicerecon/reconstruction/utils.hpp"


namespace slicerecon {

Reconstructor::Reconstructor(const Settings& param, const Geometry& geom)
     : param_(param), geom_(geom) {
    float_param_["lambda"] = &param_.paganin.lambda;
    float_param_["delta"] = &param_.paganin.delta;
    float_param_["beta"] = &param_.paganin.beta;
    float_param_["distance"] = &param_.paganin.distance;
    bool_param_["retrieve phase"] = &param_.retrieve_phase;

    initialize();
    
    projection_processor_ = std::make_unique<ProjectionProcessor>(
        geom_.rows, geom_.cols, param.filter_cores);

    projection_processor_->flatfielder =
        std::make_unique<detail::Flatfielder>(detail::Flatfielder{
            {dark_avg_.data(), geom_.rows, geom_.cols},
            {reciprocal_.data(), geom_.rows, geom_.cols}});

    if (!param_.already_linear && !param_.retrieve_phase) {
        projection_processor_->neglog =
            std::make_unique<detail::Neglogger>(detail::Neglogger{});
    }

    projection_processor_->filterer = std::make_unique<detail::Filterer>(
        detail::Filterer{param_, geom_, &buffer_[0]});

    if (!geom_.parallel) {
        projection_processor_->fdk_scale =
            std::make_unique<detail::FDKScaler>(detail::FDKScaler{
                ((ConeBeamSolver*)(solver_.get()))->fdk_weights()});
    }

    if (param_.retrieve_phase) {
        projection_processor_->paganin = std::make_unique<detail::Paganin>(
            detail::Paganin{param_, geom_, &buffer_[0]});
    }
}

Reconstructor::~Reconstructor() = default;

void Reconstructor::addListener(Listener* l) {
    listeners_.push_back(l);
    l->register_(this);

    for (auto [k, v] : float_param_) {
        l->register_parameter(k, *v);
    }
    for (auto [k, v] : bool_param_) {
        l->register_parameter(k, *v);
    }
}

void Reconstructor::pushProjection(ProjectionType k, 
                                   int32_t proj_idx, 
                                   const std::array<int32_t, 2>& shape, 
                                   char* data) {
    auto p = param_;

    if (shape[0] != geom_.rows || shape[1] != geom_.cols) {
        spdlog::error("Received projection with wrong shape. Actual: {} x {}, expected: {} x {}", 
                      shape[0], shape[1], geom_.rows, geom_.cols);
        throw std::runtime_error(
            "Received projection has a different shape than the one set by "
            "the acquisition geometry");
    }

    switch (k) {
        case ProjectionType::standard: {
            if (received_darks_ > 0 && received_flats_ > 0) {
                if (received_darks_ < p.darks || received_flats_ < p.flats) {
                    spdlog::warn("Computing reciprocal with less darks and/or flats than expected. "
                                 "Received: {}/{}, Expected: {}/{} ...", 
                                 received_darks_, received_flats_, p.darks, p.flats);
                }
                utils::computeReciprocal(all_darks_, all_flats_, pixels_, reciprocal_, dark_avg_);
                reciprocal_computed_ = true;
                received_darks_ = 0;
                received_flats_ = 0;
            }
            if (!reciprocal_computed_) {
                spdlog::warn("Send dark and flat images first! Projection ignored.");
                return;
            }

            int32_t rel_idx = proj_idx % buffer_size_;
            bool group_end_reached = rel_idx % p.group_size == p.group_size - 1;
            bool buffer_end_reached = rel_idx == buffer_size_ - 1;
 
            // buffer incoming
            auto pos = data;
            for (int i = 0, buf_idx = rel_idx * pixels_; i < pixels_; ++i, ++buf_idx) {
                raw_dtype v;
                memcpy(&v, pos, sizeof(raw_dtype));
                pos += sizeof(raw_dtype);
                buffer_[buf_idx] = static_cast<float>(v);
            }

            if (group_end_reached || buffer_end_reached) {
                auto begin_in_buffer = rel_idx - (rel_idx % p.group_size);
                processProjections(begin_in_buffer, rel_idx);
            }

            if (buffer_end_reached) {
                if (p.recon_mode == ReconstructMode::alternating) {
                    transposeIntoSino(0, buffer_size_ - 1);

                    active_gpu_buffer_index_ = 1 - active_gpu_buffer_index_;
                    bool use_gpu_lock = false;

                    uploadSinoBuffer(0, buffer_size_ - 1, active_gpu_buffer_index_, use_gpu_lock);

                } else { // --continuous mode
                    auto begin_wrt_geom = (update_count_ * buffer_size_) % geom_.projections;
                    auto end_wrt_geom = (begin_wrt_geom + buffer_size_ - 1) % geom_.projections;
                    bool use_gpu_lock = true;
                    int gpu_buffer_idx = 0; // we only have one buffer

                    if (end_wrt_geom > begin_wrt_geom) {
                        transposeIntoSino(0, buffer_size_ - 1);
                        uploadSinoBuffer(begin_wrt_geom, end_wrt_geom, gpu_buffer_idx, use_gpu_lock);
                    } else {
                        transposeIntoSino(0, geom_.projections - 1 - begin_wrt_geom);
                        // we have gone around in the geometry
                        uploadSinoBuffer(begin_wrt_geom, geom_.projections - 1, gpu_buffer_idx, use_gpu_lock);

                        transposeIntoSino(geom_.projections - begin_wrt_geom, buffer_size_ - 1);
                        uploadSinoBuffer(0, end_wrt_geom, gpu_buffer_idx, use_gpu_lock);
                    }

                    ++update_count_;
                }

                // update low-quality 3D reconstruction
                refreshData();
            }

            break;
        }
        case ProjectionType::dark: {
            reciprocal_computed_ = false;
            if (received_darks_ == p.darks) {
                spdlog::warn("Received more darks than expected. New dark ignored!");
                return;
            }
            memcpy(&all_darks_[received_darks_ * pixels_], data, sizeof(raw_dtype) * pixels_);
            ++received_darks_;
            spdlog::info("Received dark No. {0:d}", received_darks_);
            break;
        }
        case ProjectionType::flat: {
            reciprocal_computed_ = false;
            if (received_flats_ == p.flats) {
                spdlog::warn("Received more flats than expected. New flat ignored!");
                return;
            }
            memcpy(&all_flats_[received_flats_ * pixels_], data, sizeof(raw_dtype) * pixels_);
            ++received_flats_;
            spdlog::info("Received flat No. {0:d}", received_flats_);
            break;
        }
        default:
            break;
    }
}

slice_data Reconstructor::reconstructSlice(orientation x) {
    // the lock is supposed to be always open if reconstruction mode ==
    // alternating
    std::lock_guard<std::mutex> guard(gpu_mutex_);

    return solver_->reconstruct_slice(x, active_gpu_buffer_index_);
}

std::vector<float>& Reconstructor::previewData() { return small_volume_buffer_; }

Settings Reconstructor::parameters() const { return param_; }

void Reconstructor::parameterChanged(std::string name, std::variant<float, std::string, bool> value) {
    if (solver_->parameter_changed(name, value)) {
        for (auto l : listeners_) {
            l->notify(*this);
        }
    }

    std::visit(
        [&](auto&& x) {
            std::cout << "Param " << name << " changed to " << x << "\n";
        },
        value);
}

std::vector<float> Reconstructor::defaultAngles(int n) {
    auto angles = std::vector<float>(n, 0.0f);
    std::iota(angles.begin(), angles.end(), 0.0f);
    std::transform(angles.begin(), angles.end(), angles.begin(),
                   [n](auto x) { return (x * M_PI) / n; });
    return angles;
}

void Reconstructor::initialize() {
    if (geom_.angles.empty()) {
        geom_.angles = Reconstructor::defaultAngles(geom_.projections);
        spdlog::info("Default angles in radians generated: min = {}, max = {}", 
                     geom_.angles.front(), geom_.angles.back());
    }

    // init counts
    pixels_ = geom_.cols * geom_.rows;

    // allocate the buffers
    all_flats_.resize(pixels_ * param_.flats);
    all_darks_.resize(pixels_ * param_.darks);
    dark_avg_.resize(pixels_);
    reciprocal_.resize(pixels_, 1.0f);

    buffer_size_ = param_.recon_mode == ReconstructMode::alternating 
                   ? geom_.projections : param_.group_size;
    buffer_.resize((size_t)buffer_size_ * (size_t)pixels_);
    sino_buffer_.resize((size_t)buffer_size_ * (size_t)pixels_);

    small_volume_buffer_.resize(param_.preview_size * param_.preview_size * param_.preview_size);

    if (geom_.parallel) {
        solver_ = std::make_unique<ParallelBeamSolver>(param_, geom_);
    } else {
        solver_ = std::make_unique<ConeBeamSolver>(param_, geom_);
    }
}

void Reconstructor::processProjections(int proj_id_begin, int proj_id_end) {
    spdlog::info("Processing buffer between {0:d} and {1:d} ...", 
                 proj_id_begin, proj_id_end);

    auto data = &buffer_[proj_id_begin * pixels_];
    projection_processor_->process(data, proj_id_begin, proj_id_end);
}

void Reconstructor::uploadSinoBuffer(int proj_id_begin, 
                                     int proj_id_end,
                                     int buffer_idx, 
                                     bool lock_gpu) {
    spdlog::info("Uploading to buffer ({0:d}) between {1:d}/{2:d}", 
                 active_gpu_buffer_index_, proj_id_begin, proj_id_end);

    {
        if (lock_gpu) {
            std::lock_guard<std::mutex> guard(gpu_mutex_);

            astra::uploadMultipleProjections(
                solver_->proj_data(buffer_idx), &sino_buffer_[0], proj_id_begin, proj_id_end);
        } else {
            astra::uploadMultipleProjections(
                solver_->proj_data(buffer_idx), &sino_buffer_[0], proj_id_begin, proj_id_end);
        }
    }

    // send message to observers that new data is available
    for (auto l : listeners_) l->notify(*this);
}


void Reconstructor::transposeIntoSino(int proj_offset, int proj_end) {
    // major to minor: [i, j, k]
    // In projection_group we have: [projection_id, rows, cols ]
    // For sinogram we want: [rows, projection_id, cols]

    auto buffer_size = proj_end - proj_offset + 1;

    for (int i = 0; i < geom_.rows; ++i) {
        for (int j = proj_offset; j <= proj_end; ++j) {
            for (int k = 0; k < geom_.cols; ++k) {
                sino_buffer_[i * buffer_size * geom_.cols + (j - proj_offset) * geom_.cols + k] =
                    buffer_[j * geom_.cols * geom_.rows + i * geom_.cols + k];
            }
        }
    }
}

void Reconstructor::refreshData() {
    { // lock guard scope
        std::lock_guard<std::mutex> guard(gpu_mutex_);
        solver_->reconstruct_preview(small_volume_buffer_, active_gpu_buffer_index_);
    } // end lock guard scope

    spdlog::info("Reconstructed low-res preview ({0:d}).", active_gpu_buffer_index_);

    // send message to observers that new data is available
    for (auto l : listeners_) l->notify(*this);
}

const std::vector<raw_dtype>& Reconstructor::darks() const { return all_darks_; }

const std::vector<raw_dtype>& Reconstructor::flats() const { return all_flats_; }

const std::vector<float>& Reconstructor::buffer() const { return buffer_; }

} // namespace slicerecon
