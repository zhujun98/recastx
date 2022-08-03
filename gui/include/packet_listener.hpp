#pragma once

#include <vector>

#include "tomop/tomop.hpp"

namespace gui {

class PacketListener {
  public:
    virtual void handle(tomop::Packet& packet) = 0;
};

// SceneList (listeners: Server)
// SceneObject (listeners: SceneList)
class PacketPublisher {

    std::vector<PacketListener*> listeners_;

  public:
    void send(tomop::Packet& packet) {
        for (auto listener : listeners_) listener->handle(packet);
    }

    void addListener(PacketListener* listener) {
        listeners_.push_back(listener);
    }
};

}  // namespace gui
