#pragma once

#include <vector>

#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class PacketPublisher {

    std::vector<PacketPublisher*> children_;

  public:

    virtual void send(Packet& packet);

    void addPublisher(PacketPublisher* pub);
};

}  // tomcat::gui
