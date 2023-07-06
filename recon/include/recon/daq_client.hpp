/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_DAQCLIENT_H
#define RECON_DAQCLIENT_H

#include <optional>
#include <queue>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "common/config.hpp"

namespace recastx::recon {

enum class ProjectionType : int { DARK = 0, FLAT = 1, PROJECTION = 2, UNKNOWN = 99 };

struct Projection {
    ProjectionType type;
    size_t index;
    size_t col_count;
    size_t row_count;
    zmq::message_t data;

    Projection(ProjectionType type, size_t index, size_t col_count, size_t row_count, zmq::message_t data) :
        type(type), index(index), col_count(col_count), row_count(row_count), data(std::move(data)) {}

};

class DaqClient {

public:

    using BufferType = std::queue<Projection>;

    static constexpr size_t K_MAX_QUEUE_SIZE = 100;
    static constexpr size_t K_MONITOR_EVERY = 1000;

private:

    zmq::context_t context_;
    zmq::socket_t socket_;

    BufferType queue_;

    zmq::socket_type parseSocketType(const std::string& socket_type) const; 

    bool running_ = false;
    bool acquiring_ = false;

    bool initialized_ = false;
    size_t num_rows_;
    size_t num_cols_;

    size_t projection_received_ = 0;

public:

    DaqClient(const std::string& endpoint, const std::string& socket_type);

    ~DaqClient();

    void start();

    void stopAcquiring();
    void startAcquiring();

    std::optional<Projection> next();
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENT_H