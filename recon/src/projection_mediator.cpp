/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "recon/projection_mediator.hpp"

namespace recastx::recon {

ProjectionMediator::ProjectionMediator(int capacity)
     : monitor_every_{1}, 
       proj_id_{0},
       queue_(capacity) {
}

void ProjectionMediator::push(DataType proj) {
    if (proj.index % monitor_every_ == proj_id_) {
        queue_.push(std::move(proj));
    }
}

bool ProjectionMediator::waitAndPop(DataType& proj, int timeout) {
    return queue_.waitAndPop(proj, timeout);
}

void ProjectionMediator::setFilter(size_t monitor_every) {
    monitor_every_ = monitor_every;
}

size_t ProjectionMediator::setId(size_t id) {
    proj_id_ = id < monitor_every_ ? id : monitor_every_ - 1;
    return proj_id_;
}

} // namespace recastx::recon