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

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "daq_client_interface.hpp"
#include "recon/queue.hpp"
#include "recon/projection.hpp"

namespace recastx::recon {

class ZmqDaqClient : public DaqClientInterface {

    bool initialized_ = false;
    size_t num_rows_;
    size_t num_cols_;

    std::atomic<size_t> projection_received_ = 0;

    [[nodiscard]] zmq::socket_type parseSocketType(const std::string& socket_type) const;

    void monitor(const Projection<>& proj);

  protected:

    ThreadSafeQueue<Projection<>> buffer_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::mutex mtx_;

    bool running_ = false;
    bool acquiring_ = false;

    virtual std::optional<nlohmann::json> parseMeta(const zmq::message_t& msg);

    virtual std::optional<Projection<>> parseData(const nlohmann::json& meta, const zmq::message_t& msg) = 0;

    bool isDataShapeValid(size_t num_rows, size_t num_cols);
  
  public:

    ZmqDaqClient(const std::string& endpoint, const std::string& socket_type, size_t concurrency);

    ~ZmqDaqClient() override;

    void stopAcquiring() override;
    void startAcquiring() override;

    void start() override;

    [[nodiscard]] bool next(Projection<>& proj) override;
};

} // namespace recastx::recon

#endif // RECON_ZMQDAQCLIENT_H