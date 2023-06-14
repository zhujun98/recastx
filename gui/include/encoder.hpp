#ifndef GUI_ENCODER_H
#define GUI_ENCODER_H

#include "message.pb.h"

namespace recastx::gui {

template<typename T>
inline Message createSetSlicePacket(uint64_t timestamp, const T& orientation) {
    Message msg;
    auto packet = msg.mutable_recon()->mutable_set_slice();
    packet->set_timestamp(timestamp);
    for (auto v : orientation) packet->add_orientation(v);
    return msg;
}

inline Message createSetImageProcParamPacket(uint32_t downsampling_col,
                                             uint32_t downsampling_row) {
    Message msg;
    auto packet = msg.mutable_param()->mutable_image_proc();
    packet->set_downsampling_col(downsampling_col);
    packet->set_downsampling_row(downsampling_row);
    return msg;
}

} // namespace recastx::gui

#endif //GUI_ENCODER_H
