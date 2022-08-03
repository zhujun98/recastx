#include "packet_publisher.hpp"


namespace gui {

void PacketPublisher::sendPacket(tomop::Packet& /*packet*/) {}

void PacketPublisher::send(tomop::Packet& packet) {
    sendPacket(packet);
    for (auto child : children_) child->send(packet);
}

void PacketPublisher::addPublisher(PacketPublisher* pub) {
    children_.push_back(pub);
}

}  // namespace gui