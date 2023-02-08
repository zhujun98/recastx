#include <cassert>

#include "recon/slice_mediator.hpp"
#include "recon/reconstructor.hpp"


namespace tomcat::recon {

SliceMediator::SliceMediator() : all_slices_(NUM_SLICES), ondemand_slices_(NUM_SLICES, true) {}

SliceMediator::~SliceMediator() = default;

void SliceMediator::resize(const std::array<size_t, 2>& shape) {
    all_slices_.resize(shape);
    ondemand_slices_.resize(shape);
}

void SliceMediator::insert(size_t timestamp, const Orientation& orientation) {
    std::lock_guard<std::mutex> lck(mtx_);
    size_t sid = timestamp % NUM_SLICES;
    params_[sid] = std::make_pair(timestamp, orientation);
    updated_.insert(sid);
    spdlog::debug("Orientation of slice {} ({}) updated", sid, timestamp);
}

void SliceMediator::reconAll(Reconstructor* recon, int gpu_buffer_index) {
    {
        std::lock_guard<std::mutex> lck(mtx_);

        assert(params_.size() == NUM_SLICES);
        for (const auto& [sid, param] : params_) {
            auto& slice = all_slices_.back()[sid];
            recon->reconstructSlice(param.second, gpu_buffer_index, std::get<2>(slice));
            std::get<1>(slice) = param.first;
        }

        updated_.clear();
    }

    all_slices_.prepare();
}

void SliceMediator::reconOnDemand(Reconstructor* recon, int gpu_buffer_index) {
    if (!updated_.empty()) {
        {
            std::lock_guard<std::mutex> lck(mtx_);

            for (auto sid : updated_) {
                auto& slice = ondemand_slices_.back()[sid];
                auto& param = params_[sid];
                recon->reconstructSlice(param.second, gpu_buffer_index, std::get<2>(slice));
                std::get<1>(slice) = param.first;
                std::get<0>(slice) = true;
            }

            updated_.clear();
        }

        ondemand_slices_.prepare();
    }
}

} // tomcat::recon