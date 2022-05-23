#pragma once

#include <vector>

#include "tomop/tomop.hpp"

namespace gui {

class PacketListener {
  public:
    virtual void handle(tomop::Packet& packet) = 0;
};

class PacketPublisher {
  public:
    void send(tomop::Packet& packet) {
        for (auto listener : listeners_) listener->handle(packet);
    }

    void add_listener(PacketListener* listener) {
        listeners_.push_back(listener);
    }

  private:
    std::vector<PacketListener*> listeners_;
};

}  // namespace gui
