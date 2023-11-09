/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_SLICEMEDIATOR_H
#define RECON_SLICEMEDIATOR_H

#include <map>
#include <unordered_set>

#include "buffer.hpp"
#include "reconstructor_interface.hpp"


namespace recastx::recon {

class SliceMediator {

public:

    using ParamType = std::map<size_t, std::pair<size_t, Orientation>>;

protected:

    ParamType params_;
    SliceBuffer<float> all_slices_;
    SliceBuffer<float, true> ondemand_slices_;
    std::unordered_set<size_t> updated_;

    std::mutex mtx_;

public:

    SliceMediator();

    ~SliceMediator();

    SliceMediator(const SliceMediator& other) = delete;
    SliceMediator& operator=(const SliceMediator& other) = delete;

    SliceMediator(SliceMediator&& other) noexcept = delete;
    SliceMediator& operator=(SliceMediator&& other) noexcept = delete;

    void resize(const SliceBuffer<float>::ShapeType& shape);

    void update(size_t timestamp, const Orientation& orientation);

    void reconAll(Reconstructor* recon, int gpu_buffer_index);

    void reconOnDemand(Reconstructor* recon, int gpu_buffer_index);

    SliceBuffer<float>& allSlices() { return all_slices_; }

    SliceBuffer<float, true>& onDemandSlices() { return ondemand_slices_; }

    [[nodiscard]] const ParamType& params() const { return params_; }
};

} // namespace recastx::recon

#endif // RECON_SLICEMEDIATOR_H