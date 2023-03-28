#ifndef GUI_ENCODER_H
#define GUI_ENCODER_H

#include "reconstruction.pb.h"

namespace recastx::gui {

template<typename T>
inline ReconRequestPacket createSetSlicePacket(uint64_t timestamp, const T& orientation) {
    ReconRequestPacket packet;
    auto request = packet.mutable_set_slice();
    request->set_timestamp(timestamp);
    for (auto v : orientation) request->add_orientation(v);
    return packet;
}

} // namespace recastx::gui

#endif //GUI_ENCODER_H
