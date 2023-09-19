/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_ZMQDAQCLIENT_H
#define RECON_ZMQDAQCLIENT_H

#include <memory>
#include <string>
#include <thread>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "daq_client_interface.hpp"
#include "recon/projection.hpp"

namespace recastx::recon {

class ZmqDaqClient : public DaqClientInterface {

    bool initialized_ = false;
    size_t num_rows_;
    size_t num_cols_;

    size_t projection_received_ = 0;

    [[nodiscard]] zmq::socket_type parseSocketType(const std::string& socket_type) const;

    void monitor(const Projection<>& proj);

  public:

    static constexpr size_t K_MAX_QUEUE_SIZE = 100;
    static constexpr size_t K_MONITOR_EVERY = 1000;

    using BufferType = std::queue<Projection<>>;

  protected:

    BufferType queue_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    size_t max_concurrency_;

    bool running_ = false;
    bool acquiring_ = false;

    bool isDataShapeValid(size_t num_rows, size_t num_cols);
  
  public:

    ZmqDaqClient(const std::string& endpoint, const std::string& socket_type, size_t max_concurrency);

    ~ZmqDaqClient() override;

    void stopAcquiring() override;
    void startAcquiring() override;

    void start() override;

    [[nodiscard]] std::optional<Projection<>> next() override;

    virtual std::optional<Projection<>> recv() = 0;
};

} // namespace recastx::recon

#endif // RECON_ZMQDAQCLIENT_H