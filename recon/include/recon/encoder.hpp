#ifndef RECON_ENCODER_H
#define RECON_ENCODER_H

#include "buffer.hpp"
#include "slice_mediator.hpp"
#include "reconstruction.pb.h"

namespace recastx::recon {

template<typename T>
inline ReconDataPacket createSliceDataPacket(const T& data, uint32_t x, uint32_t y, uint64_t timestamp) {
    ReconDataPacket packet;
    auto slice = packet.mutable_slice();
    // TODO: change 'float' to 'ValueType'
    slice->set_data(data.data(), data.size() * sizeof(float));
    slice->set_col_count(x);
    slice->set_row_count(y);
    slice->set_timestamp(timestamp);
    return packet;
}

template<typename T>
inline ReconDataPacket createVolumeDataPacket(const T& data, uint32_t x, uint32_t y, uint32_t z) {
    ReconDataPacket packet;
    auto volume = packet.mutable_volume();
    // TODO: change 'float' to 'ValueType'
    volume->set_data(data.data(), data.size() * sizeof(float));
    volume->set_col_count(x);
    volume->set_row_count(y);
    volume->set_slice_count(z);
    return packet;
}

} // namespace recastx::recon

#endif // RECON_ENCODER_H