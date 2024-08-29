/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>
#include <chrono>
#include <iostream>

#include <spdlog/spdlog.h>

#include "common/config.hpp"
#include "recon/daq/zmq_daq_client.hpp"

namespace recastx::recon {

ZmqDaqClient::ZmqDaqClient(const std::string& endpoint, const std::string& socket_type, size_t concurrency)
        : DaqClientInterface(concurrency),
          buffer_(k_DAQ_BUFFER_SIZE),
          context_(1),
          socket_(context_, parseSocketType(socket_type)) {
    socket_.set(zmq::sockopt::rcvtimeo, 100);
    socket_.set(zmq::sockopt::rcvhwm, 1);
    socket_.connect(endpoint);

    if(socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::sub)) {
        spdlog::info("[DAQ client] Connected to data server (PUB-SUB) at {}", endpoint);
        socket_.set(zmq::sockopt::subscribe, "");
    } else if (socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::pull)) {
        spdlog::info("[DAQ client] Connected to data server (PUSH-PULL) {}", endpoint);
    }
}

ZmqDaqClient::~ZmqDaqClient() {
    running_ = false;
    socket_.set(zmq::sockopt::linger, 200);
}

void ZmqDaqClient::spin() {
    if (running_) {
        spdlog::warn("[DAQ client] DAQ client is already running!");
        return;
    }

    spdlog::info("[DAQ client] Starting DAQ client (concurrency = {})", concurrency_);

    running_ = true;
    for (size_t i = 0; i < concurrency_; ++i) {
        auto t = std::thread([&] {
            zmq::message_t update;
            std::optional<nlohmann::json> meta;
            while (running_) {
                if (!acquiring_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                {
                    std::lock_guard lk(mtx_);
                    auto result = socket_.recv(update, zmq::recv_flags::none);
                    if (!result) continue;

                    assert(update.more());

                    meta = parseMeta(update);
                    if (!meta) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }

                    while (running_) {
                        auto result = socket_.recv(update, zmq::recv_flags::none);
                        if (result.has_value() || !acquiring_) break;
                    }
                }

                assert(!update.more());

                auto data = parseData(meta.value(), update);
                if (!data) continue;

                while (running_) {
                    if (buffer_.tryPush(data.value())) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        });

        t.detach();
    }
}

bool ZmqDaqClient::next(Projection<>& proj) {
    return buffer_.waitAndPop(proj, 100);
}

void ZmqDaqClient::setAcquiring(bool state) {
    DaqClientInterface::setAcquiring(state);
    if (state) buffer_.reset();
    spdlog::debug("Zmq buffer reset!");
}

void ZmqDaqClient::startAcquiring(uint32_t num_rows, uint32_t num_cols) {
    DaqClientInterface::startAcquiring(num_rows, num_cols);
    buffer_.reset();
    spdlog::debug("Zmq buffer reset!");
}

std::optional<nlohmann::json> ZmqDaqClient::parseMeta(const zmq::message_t& msg) {
    try {
        return nlohmann::json::parse(msg.to_string());
    } catch (const nlohmann::json::parse_error& ex) {
        spdlog::error("[DAQ client] Failed to parse metadata: {}", ex.what());
        return std::nullopt;
    }
}

zmq::socket_type ZmqDaqClient::parseSocketType(const std::string& socket_type) const {
    if (socket_type == "pull") return zmq::socket_type::pull;
    if (socket_type == "sub") return zmq::socket_type::sub;
    throw std::invalid_argument(fmt::format("Unsupported socket type: {}", socket_type)); 
}

} // namespace recastx::recon