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

inline Message createSetStatePacket(StatePacket_State state) {
    Message msg;
    auto packet = msg.mutable_state();
    packet->set_state(state);
    return msg;
}

} // namespace recastx::gui

#endif //GUI_ENCODER_H
