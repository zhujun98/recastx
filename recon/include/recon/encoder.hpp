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
inline std::vector<rpc::ReconData> createVolumeDataPacket(const Container& data, uint32_t x, uint32_t y, uint32_t z) {
    std::vector<rpc::ReconData> packets;
    auto* ptr = data.data();
    uint32_t shard_size = x * y;
    for (uint32_t i = 0; i < z; ++i) {
        rpc::ReconData packet;
        auto shard = packet.mutable_volume_shard();
        shard->set_data(ptr, shard_size * sizeof(typename Container::value_type));
        shard->set_col_count(x);
        shard->set_row_count(y);
        shard->set_slice_count(z);
        shard->set_pos(i * shard_size);
        packets.emplace_back(std::move(packet));

        std::advance(ptr, shard_size);
    }
    return packets;
}

template<typename Container>
inline rpc::ProjectionData createProjectionDataPacket(uint32_t id, uint32_t x, uint32_t y, const Container& data) {
    rpc::ProjectionData packet;
    packet.set_id(id);
    packet.set_col_count(x);
    packet.set_row_count(y);
    packet.set_data(data.data(), data.size() * sizeof(typename Container::value_type));
    return packet;
}

} // namespace recastx::recon

#endif // RECON_ENCODER_H