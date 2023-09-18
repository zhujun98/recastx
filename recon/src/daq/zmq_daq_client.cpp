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

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "recon/daq/zmq_daq_client.hpp"

namespace recastx::recon {

ZmqDaqClient::ZmqDaqClient(const std::string& endpoint, const std::string& socket_type, size_t max_concurrency)
        : DaqClientInterface(),
          context_(max_concurrency),
          socket_(context_, parseSocketType(socket_type)),
          max_concurrency_(max_concurrency) {
    socket_.connect(endpoint);

    if(socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::sub)) {
        spdlog::info("Connected to data server (PUB-SUB) at {}", endpoint);
        socket_.set(zmq::sockopt::subscribe, "");
    } else if (socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::pull)) {
        spdlog::info("Connected to data server (PUSH-PULL) {}", endpoint);
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
        spdlog::warn("DAQ client is already running!");
        return;
    }

    auto t = std::thread([&] {
        running_ = true;
        while (running_) {
            if (!acquiring_ || queue_.size() == K_MAX_QUEUE_SIZE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            auto data = recv();
            if (!data.has_value()) continue;

#if (VERBOSITY >= 1)
            monitor(data.value());
#endif

            queue_.push(std::move(data.value()));
        }
    });

    t.detach();
}

std::optional<Projection<>> ZmqDaqClient::next() {
    if (queue_.empty()) return std::nullopt;

    auto data = std::move(queue_.front());
    queue_.pop();
    return data;
}

zmq::socket_type ZmqDaqClient::parseSocketType(const std::string& socket_type) const {
    if (socket_type == "pull") return zmq::socket_type::pull;
    if (socket_type == "sub") return zmq::socket_type::sub;
    throw std::invalid_argument(fmt::format("Unsupported socket type: {}", socket_type)); 
}

bool ZmqDaqClient::isDataShapeValid(size_t num_rows, size_t num_cols) {
    if (initialized_) {
        if (num_rows != num_rows_ || num_cols != num_cols_) {
            // monitor only
            spdlog::warn("Received image data with a different shape. Current: {} x {}, "
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

void ZmqDaqClient::monitor(const Projection<>& proj) {
    if (proj.type == ProjectionType::PROJECTION) {
        ++projection_received_;
        if (projection_received_ % K_MONITOR_EVERY == 0) {
            spdlog::info("# of projections received: {}", projection_received_);
        }                
    } else {
        // reset the counter when receiving a dark or a flat
        projection_received_ = 0;
    }
}

} // namespace recastx::recon