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

namespace recastx::recon {

enum class ProjectionType : int { DARK = 0, FLAT = 1, PROJECTION = 2, UNKNOWN = 99 };

struct ProjectionMessage {
    ProjectionType type;
    size_t index;
    size_t col_count;
    size_t row_count;
    zmq::message_t data;

    ProjectionMessage(ProjectionType type, size_t index, size_t col_count, size_t row_count, zmq::message_t data) :
        type(type), index(index), col_count(col_count), row_count(row_count), data(std::move(data)) {}

};

class DaqClientInterface {

  public:

    using BufferType = std::queue<ProjectionMessage>;

  protected:

    BufferType queue_;

  public:

    virtual ~DaqClientInterface() = default;

    virtual void start() = 0;

    virtual void startAcquiring() = 0;
    virtual void stopAcquiring() = 0;

    std::optional<ProjectionMessage> next() {
        if (queue_.empty()) return {};

        auto ret = std::move(queue_.front());
        queue_.pop();
        return ret;
    }
};

} // namespace recastx::recon

#endif // RECON_DAQCLIENTINTERFACE_H