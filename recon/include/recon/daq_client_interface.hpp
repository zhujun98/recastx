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

#include <zmq.hpp>

#include "projection.hpp"

namespace recastx::recon {

class DaqClientInterface {

  public:

    using BufferType = std::queue<Projection<>>;

  protected:

    BufferType queue_;

  public:

    virtual ~DaqClientInterface() = default;

    virtual void start() = 0;

    virtual void startAcquiring() = 0;
    virtual void stopAcquiring() = 0;

    [[nodiscard]] std::optional<Projection<>> next() {
        if (queue_.empty()) return std::nullopt;

        auto data = std::move(queue_.front());
        queue_.pop();
        return data;
    }
};

} // namespace recastx::recon

#endif // RECON_DAQCLIENTINTERFACE_H