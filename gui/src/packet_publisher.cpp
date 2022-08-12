#include "packet_publisher.hpp"


namespace tomcat::gui {

void PacketPublisher::sendPacket(Packet& /*packet*/) {}

void PacketPublisher::send(Packet& packet) {
    sendPacket(packet);
    for (auto child : children_) child->send(packet);
}

void PacketPublisher::addPublisher(PacketPublisher* pub) {
    children_.push_back(pub);
}

}  // tomcat::gui