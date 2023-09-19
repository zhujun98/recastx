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

#include "recon/daq/std_daq_client.hpp"

namespace recastx::recon {

StdDaqClient::StdDaqClient(const std::string& endpoint, const std::string& socket_type, size_t max_concurrency)
        : ZmqDaqClient(endpoint, socket_type, max_concurrency) {}

StdDaqClient::~StdDaqClient() = default; 

std::optional<Projection<>> StdDaqClient::recv() {
    zmq::message_t update;

    // metadata
    auto result = socket_.recv(update, zmq::recv_flags::dontwait);
    if (!result.has_value()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return std::nullopt;
    }
    auto meta = nlohmann::json::parse(update.to_string());
    size_t frame = meta["frame"];
    int scan_index = meta["image_attributes"]["scan_index"]; 
    ProjectionType proj_type = parseProjectionType(scan_index);
    if (proj_type == ProjectionType::UNKNOWN) {
        spdlog::error("Unknown scan index: {}", scan_index);
        return std::nullopt;;
    }
    size_t num_rows = meta["shape"][0];
    size_t num_cols = meta["shape"][1];
    if (!isDataShapeValid(num_rows, num_cols)) return std::nullopt;

    // data
    socket_.recv(update, zmq::recv_flags::none);
    assert(update.size() == sizeof(RawDtype) * n_rows * n_cols);
    
    return Projection<>{proj_type, frame, num_cols, num_rows, update.data(), update.size()};
}

} // namespace recastx::recon