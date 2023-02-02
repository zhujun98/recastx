#include "recon/slice_mediator.hpp"
#include "recon/reconstructor.hpp"


namespace tomcat::recon {

SliceMediator::SliceMediator() : all_slices_(NUM_SLICES), requested_slices_(NUM_SLICES, false) {}

SliceMediator::~SliceMediator() = default;

void SliceMediator::resize(const std::array<size_t, 2>& shape) {
    all_slices_.resize(shape);
    requested_slices_.resize(shape);
}

void SliceMediator::insert(size_t timestamp, const Orientation& orientation) {
    std::lock_guard<std::mutex> lck(mtx_);
    size_t sid = timestamp % NUM_SLICES;
    params_[sid] = std::make_pair(timestamp, orientation);
    updated_.insert(sid);
}

void SliceMediator::reconAll(Reconstructor* recon, int gpu_buffer_index) {
    {
        std::lock_guard<std::mutex> lck(mtx_);

        for (const auto& [sid, param] : params_) {
            auto& slice = all_slices_.back().second[sid];
            recon->reconstructSlice(param.second, gpu_buffer_index, slice.second);
            slice.first = param.first;
        }

        updated_.clear();
    }

    all_slices_.prepare();
}

void SliceMediator::reconRequested(Reconstructor* recon, int gpu_buffer_index) {
    if (!updated_.empty()) {
        {
            std::lock_guard<std::mutex> lck(mtx_);

            for (auto sid : updated_) {
                auto slice = requested_slices_.back().second[sid];
                auto param = params_[sid];
                recon->reconstructSlice(param.second, gpu_buffer_index, slice.second);
                slice.first = param.first;
                requested_slices_.back().first.emplace(sid);
            }

            updated_.clear();
        }

        requested_slices_.prepare();
    }
}

} // tomcat::recon