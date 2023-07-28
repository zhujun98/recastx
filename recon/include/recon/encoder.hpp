/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_ENCODER_H
#define RECON_ENCODER_H

#include "buffer.hpp"
#include "slice_mediator.hpp"
#include "projection.pb.h"
#include "reconstruction.pb.h"

namespace recastx::recon {

template<typename Container>
inline rpc::ReconData createSliceDataPacket(const Container& data, uint32_t x, uint32_t y, uint64_t timestamp) {
    rpc::ReconData packet;
    auto slice = packet.mutable_slice();
    slice->set_data(data.data(), data.size() * sizeof(typename Container::value_type));
    slice->set_col_count(x);
    slice->set_row_count(y);
    slice->set_timestamp(timestamp);
    return packet;
}

template<typename Container>
inline rpc::ReconData createVolumeDataPacket(const Container& data, uint32_t x, uint32_t y, uint32_t z) {
    rpc::ReconData packet;
    auto volume = packet.mutable_volume();
    volume->set_data(data.data(), data.size() * sizeof(typename Container::value_type));
    volume->set_col_count(x);
    volume->set_row_count(y);
    volume->set_slice_count(z);
    return packet;
}

template<typename Container>
inline rpc::ProjectionData createProjectionDataPacket(const Container& data, uint32_t x, uint32_t y) {
    rpc::ProjectionData packet;
     packet.set_data(data.data(), data.size() * sizeof(typename Container::value_type));
    packet.set_col_count(x);
    packet.set_row_count(y);
    return packet;
}

} // namespace recastx::recon

#endif // RECON_ENCODER_H