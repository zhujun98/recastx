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

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "recon/daq_client.hpp"


namespace recastx::recon {

using namespace std::string_literals;

namespace detail {

ProjectionType parseProjectionType(int v) {
    if (v != static_cast<int>(ProjectionType::DARK) &&
        v != static_cast<int>(ProjectionType::FLAT) && 
        v != static_cast<int>(ProjectionType::PROJECTION)) {
            return ProjectionType::UNKNOWN;
        }
    return static_cast<ProjectionType>(v);
}

} // detail


DaqClient::DaqClient(const std::string& endpoint, const std::string& socket_type)
        : DaqClientInterface(),
          context_(1),
          socket_(context_, parseSocketType(socket_type)) {
    socket_.connect(endpoint);

    if(socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::sub)) {
        spdlog::info("Connected to data server (PUB-SUB) at {}", endpoint);
        socket_.set(zmq::sockopt::subscribe, "");
    } else if (socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::pull)) {
        spdlog::info("Connected to data server (PUSH-PULL) {}", endpoint);
    }
}

DaqClient::~DaqClient() {
    running_ = false;
    socket_.set(zmq::sockopt::linger, 200);
}

void DaqClient::start() {
    if (running_) {
        throw std::runtime_error("Cannot start a DaqClient twice!");
    }

    auto t = std::thread([&] {
        zmq::message_t update;

        running_ = true;
        while (running_) {
            // Caveat: sequence of conditions
            if (!acquiring_ 
                    || queue_.size() == K_MAX_QUEUE_SIZE 
                    || !socket_.recv(update, zmq::recv_flags::dontwait).has_value()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            auto meta = nlohmann::json::parse(update.to_string());
            size_t frame = meta["frame"];
            int scan_index = meta["image_attributes"]["scan_index"]; 
            ProjectionType proj_type = detail::parseProjectionType(scan_index);
            if (proj_type == ProjectionType::UNKNOWN) {
                spdlog::error("Unknown scan index: {}", scan_index);
                continue;
            }

            size_t n_rows = meta["shape"][0];
            size_t n_cols = meta["shape"][1];
            if (initialized_) {
                if (n_rows != num_rows_ || n_cols != num_cols_) {
                    // monitor only
                    spdlog::warn("Received image data with a different shape. Current: {} x {}, "
                                 "before: {} x {}", n_rows, n_cols, num_rows_, num_cols_);
                    continue;
                }
            } else {
                num_rows_ = n_rows;
                num_cols_ = n_cols;
                initialized_ = true;
            }

            socket_.recv(update, zmq::recv_flags::none);
            assert(update.size() == sizeof(RawDtype) * n_rows * n_cols);
            queue_.emplace(proj_type, frame, n_cols, n_rows, std::move(update));

#if (VERBOSITY >= 1)
            if (proj_type == ProjectionType::PROJECTION) {
                ++projection_received_;
                if (projection_received_ % K_MONITOR_EVERY == 0) {
                    spdlog::info("# of projections received: {}", projection_received_);
                }                
            } else {
                // reset the counter when receiving a dark or a flat
                projection_received_ = 0;
            }
#endif

        }
    });

    t.detach();
}

void DaqClient::startAcquiring() {
    acquiring_ = true;
}

void DaqClient::stopAcquiring() {
    acquiring_ = false;
}

zmq::socket_type DaqClient::parseSocketType(const std::string& socket_type) const {
    if (socket_type.compare("pull") == 0) return zmq::socket_type::pull;
    if (socket_type.compare("sub") == 0) return zmq::socket_type::sub;
    throw std::invalid_argument("Unsupported socket type: "s + socket_type); 
}

} // namespace recastx::recon