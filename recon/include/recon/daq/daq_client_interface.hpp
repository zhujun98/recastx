/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQCLIENTINTERFACE_H
#define RECON_DAQCLIENTINTERFACE_H

#include <optional>
#include <queue>

#include "recon/projection.hpp"

namespace recastx::recon {

class DaqClientInterface {

  protected:

    static ProjectionType parseProjectionType(int v) {
        if (v != static_cast<int>(ProjectionType::DARK) &&
            v != static_cast<int>(ProjectionType::FLAT) && 
            v != static_cast<int>(ProjectionType::PROJECTION)) {
                return ProjectionType::UNKNOWN;
            }
        return static_cast<ProjectionType>(v);
  }

  public:

    virtual ~DaqClientInterface() = default;

    virtual void start() = 0;

    virtual void startAcquiring() = 0;
    virtual void stopAcquiring() = 0;

    [[nodiscard]] virtual std::optional<Projection<>> next() = 0;
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENTINTERFACE_H