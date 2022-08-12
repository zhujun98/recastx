#pragma once

#include <vector>

#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class PacketPublisher {

    std::vector<PacketPublisher*> children_;

    virtual void sendPacket(Packet& /*packet*/);

  public:

    void send(Packet& packet);

    void addPublisher(PacketPublisher* pub);
};

}  // tomcat::gui
