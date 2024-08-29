/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
#include <nlohmann/json.hpp>

#include "daq_client_interface.hpp"
#include "common/queue.hpp"
#include "recon/projection.hpp"

namespace recastx::recon {

class ZmqDaqClient : public DaqClientInterface {

    [[nodiscard]] zmq::socket_type parseSocketType(const std::string& socket_type) const;

  protected:

    ThreadSafeQueue<Projection<>> buffer_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::mutex mtx_;

    bool running_ = false;

    virtual std::optional<nlohmann::json> parseMeta(const zmq::message_t& msg);

    virtual std::optional<Projection<>> parseData(const nlohmann::json& meta, const zmq::message_t& msg) = 0;

  public:

    ZmqDaqClient(const std::string& endpoint, const std::string& socket_type, size_t concurrency);

    ~ZmqDaqClient() override;

    void spin() override;

    [[nodiscard]] bool next(Projection<>& proj) override;

    void setAcquiring(bool state) override;

    void startAcquiring(uint32_t num_rows, uint32_t num_cols) override;
};

} // namespace recastx::recon

#endif // RECON_ZMQDAQCLIENT_H