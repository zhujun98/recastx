#pragma once

#include <vector>

#include "tomop/tomop.hpp"

namespace tomop::gui {

class PacketPublisher {

    std::vector<PacketPublisher*> children_;

    virtual void sendPacket(Packet& /*packet*/);

  public:

    void send(Packet& packet);

    void addPublisher(PacketPublisher* pub);
};

}  // tomop::gui
