/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "recon/projection.hpp"
#include "recon/daq_client_interface.hpp"

namespace recastx::recon {

Projection::Projection(const daq::Message& msg) :
        type(msg.type), index(msg.index), col_count(msg.col_count), row_count(msg.row_count) {
    data.resize(col_count * row_count);
    std::memcpy(data.data(), msg.data.data(), msg.data.size());
}

} // namespace recastx::recon
