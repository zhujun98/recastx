/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_PROJECTIONMEDIATOR_H
#define RECON_PROJECTIONMEDIATOR_H

#include <queue>

#include "buffer.hpp"
#include "projection.hpp"

namespace recastx::recon {

class ProjectionMediator {

    size_t monitor_every_;
    size_t monitor_index_;

    ImageBuffer<RawDtype> projections_;

  public:
    
    using ImageType = typename ImageBuffer<RawDtype>::BufferType;

    ProjectionMediator();

    ~ProjectionMediator();

    void emplace(Projection<>&& proj);

    void setFilter(int monitor_every, int monitor_index = 0);

    ImageBuffer<RawDtype>& projections() { return projections_; }
};

} // namespace recastx::recon

#endif // RECON_PROJECTIONMEDIATOR_H