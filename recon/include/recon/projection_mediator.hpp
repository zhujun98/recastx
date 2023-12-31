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

#include "common/queue.hpp"
#include "projection.hpp"

namespace recastx::recon {

class ProjectionMediator {

  public:

    using DataType = Projection<>;

  private:

    size_t monitor_every_;
    size_t proj_id_;

    ThreadSafeQueue<DataType> queue_;

  public:
    
    ProjectionMediator(int capacity = 0);

    ~ProjectionMediator();

    void push(DataType proj);

    bool waitAndPop(DataType& proj, int timeout = -1);

    void setFilter(size_t monitor_every);
    
    size_t setId(size_t id);

    void reset() { queue_.reset(); }
};

} // namespace recastx::recon

#endif // RECON_PROJECTIONMEDIATOR_H