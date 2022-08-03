#pragma once

#include <vector>

#include "tomop/tomop.hpp"

namespace gui {

class PacketPublisher {

    std::vector<PacketPublisher*> children_;

    virtual void sendPacket(tomop::Packet& /*packet*/);

  public:

    void send(tomop::Packet& packet);

    void addPublisher(PacketPublisher* pub);
};

}  // namespace gui
