/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>

#include "common/utils.hpp"
#include "recon/slice_mediator.hpp"


namespace recastx::recon {

SliceMediator::SliceMediator() = default;

SliceMediator::~SliceMediator() = default;

void SliceMediator::resize(const SliceBuffer<float>::ShapeType& shape) {
    all_slices_.resize(shape);
    ondemand_slices_.resize(shape);
}

void SliceMediator::update(size_t timestamp, const Orientation& orientation) {
    std::lock_guard<std::mutex> lck(mtx_);
    size_t sid = sliceIdFromTimestamp(timestamp);
    auto [it, inserted] = params_.insert_or_assign(sid, std::make_pair(timestamp, orientation));
    if (inserted) {
        [[maybe_unused]] bool success1 = all_slices_.insert(sid);
        assert(inserted == success1);
        [[maybe_unused]] bool success2 = ondemand_slices_.insert(sid);
        assert(inserted == success2);
    }
    updated_.insert(sid);

    assert(all_slices_.size() <= MAX_NUM_SLICES);
    spdlog::info("Slice {} orientation updated", sid);
}

void SliceMediator::reconAll(Reconstructor* recon, int gpu_buffer_index) {
    {
        std::lock_guard<std::mutex> lck(mtx_);

        for (const auto& [sid, param] : params_) {
            auto& slice = all_slices_.back()[sid];
            recon->reconstructSlice(param.second, gpu_buffer_index, std::get<2>(slice));
            std::get<1>(slice) = param.first;
        }

        updated_.clear();
    }

    if (all_slices_.prepare()) {
        spdlog::debug("Reconstructed slices dropped due to slowness of clients");
    }
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

                spdlog::debug("On-demand slice {} ({}) reconstructed", sid, param.first);
            }

            updated_.clear();
        }

        if (ondemand_slices_.prepare()) {
            spdlog::debug("On-demand reconstructed slices dropped due to slowness of clients");
        }
    }
}

} // namespace recastx::recon