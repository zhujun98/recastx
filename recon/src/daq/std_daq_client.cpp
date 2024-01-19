/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cassert>
#include <iostream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "recon/daq/std_daq_client.hpp"

namespace recastx::recon {

StdDaqClient::StdDaqClient(const std::string& endpoint, const std::string& socket_type, size_t concurrency)
        : ZmqDaqClient(endpoint, socket_type, concurrency) {}

std::optional<Projection<>>
StdDaqClient::parseData(const nlohmann::json& meta, const zmq::message_t& data) {
    size_t frame = meta["frame"];
    int scan_index = meta["image_attributes"]["scan_index"]; 
    ProjectionType proj_type = parseProjectionType(scan_index);
    if (proj_type == ProjectionType::UNKNOWN) {
        spdlog::error("Unknown scan index: {}", scan_index);
        return std::nullopt;;
    }
    size_t num_rows = meta["shape"][0];
    size_t num_cols = meta["shape"][1];
    if (!isDataShapeValid(num_rows, num_cols)) {
        return std::nullopt;
    }

    assert(data.size() == sizeof(RawDtype) * num_rows * num_cols);
    return Projection<>{proj_type, frame, num_cols, num_rows, data.data(), data.size()};
}

} // namespace recastx::recon