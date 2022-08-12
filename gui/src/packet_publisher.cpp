#include "packet_publisher.hpp"


namespace tomcat::gui {

void PacketPublisher::send(Packet& packet) {
    for (auto child : children_) child->send(packet);
}

void PacketPublisher::addPublisher(PacketPublisher* pub) {
    children_.push_back(pub);
}

}  // tomcat::gui