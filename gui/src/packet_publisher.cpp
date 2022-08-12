#include "packet_publisher.hpp"


namespace tomop::gui {

void PacketPublisher::sendPacket(Packet& /*packet*/) {}

void PacketPublisher::send(Packet& packet) {
    sendPacket(packet);
    for (auto child : children_) child->send(packet);
}

void PacketPublisher::addPublisher(PacketPublisher* pub) {
    children_.push_back(pub);
}

}  // tomop::gui