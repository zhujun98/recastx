/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>

#include "recon/projection_mediator.hpp"
#include "recon/projection.hpp"

namespace recastx::recon {

ProjectionMediator::ProjectionMediator(int capacity)
     : monitor_every_{1}, 
       monitor_index_{0},
       projections_(capacity) {
}

ProjectionMediator::~ProjectionMediator() = default;

void ProjectionMediator::emplace(Projection<>&& proj) {
    if (proj.index % monitor_every_ == monitor_index_) {
        projections_.emplace(std::move(proj.data));
    }
}

void ProjectionMediator::setFilter(int monitor_every, int monitor_index) {
    monitor_every_ = monitor_every > 0 ? static_cast<size_t>(monitor_every) : 1;
    if (monitor_index < 0) {
        monitor_index_ = 0;
    } else {
        size_t index = static_cast<size_t>(monitor_index);
        monitor_index_ = index < monitor_every_ ? index : monitor_every_ - 1;
    }
}

} // namespace recastx::recon