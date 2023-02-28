#include <cassert>

#include "common/utils.hpp"
#include "recon/slice_mediator.hpp"
#include "recon/reconstructor.hpp"


namespace tomcat::recon {

SliceMediator::SliceMediator() : ondemand_slices_(true) {}

SliceMediator::~SliceMediator() = default;

void SliceMediator::reshape(const SliceBufferType::ShapeType& shape) {
    all_slices_.reshape(shape);
    ondemand_slices_.reshape(shape);
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
    spdlog::debug("Orientation of slice {} ({}) updated", sid, timestamp);
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