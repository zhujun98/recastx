/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

void ZmqDaqClient::startAcquiring() {
    acquiring_ = true;
}

void ZmqDaqClient::stopAcquiring() {
    acquiring_ = false;
}

void ZmqDaqClient::start() {
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
                    if (!result.has_value()) continue;

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

std::optional<nlohmann::json> ZmqDaqClient::parseMeta(const zmq::message_t& msg) {
    try {
        return nlohmann::json::parse(msg.to_string());
    } catch (const nlohmann::json::parse_error& ex) {
        spdlog::error("[DAQ client] Failed to parse metadata: {}", ex.what());
        return std::nullopt;
    }
}

bool ZmqDaqClient::isDataShapeValid(size_t num_rows, size_t num_cols) {
    if (initialized_) {
        if (num_rows != num_rows_ || num_cols != num_cols_) {
            // monitor only
            spdlog::warn("[DAQ client] Received image data with a different shape. Current: {} x {}, "
                         "before: {} x {}", num_rows, num_cols, num_rows_, num_cols_);
            return false;
        }

        return true;
    }

    num_rows_ = num_rows;
    num_cols_ = num_cols;
    initialized_ = true;
    return true;
}

zmq::socket_type ZmqDaqClient::parseSocketType(const std::string& socket_type) const {
    if (socket_type == "pull") return zmq::socket_type::pull;
    if (socket_type == "sub") return zmq::socket_type::sub;
    throw std::invalid_argument(fmt::format("Unsupported socket type: {}", socket_type)); 
}

} // namespace recastx::recon