/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQCLIENTINTERFACE_H
#define RECON_DAQCLIENTINTERFACE_H

#include <queue>

#include "recon/projection.hpp"

namespace recastx::recon {

class DaqClientInterface {

  protected:

    size_t concurrency_;

    static ProjectionType parseProjectionType(int v) {
        if (v != static_cast<int>(ProjectionType::DARK) &&
            v != static_cast<int>(ProjectionType::FLAT) && 
            v != static_cast<int>(ProjectionType::PROJECTION)) {
                return ProjectionType::UNKNOWN;
            }
        return static_cast<ProjectionType>(v);
  }

  public:

    explicit DaqClientInterface(size_t concurrency) : concurrency_(concurrency) {}

    virtual ~DaqClientInterface() = default;

    virtual void spin() = 0;

    virtual void setAcquiring(bool state) = 0;

    [[nodiscard]] virtual bool next(Projection<>& proj) = 0;

    [[nodiscard]] size_t concurrency() const { return concurrency_; }
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENTINTERFACE_H