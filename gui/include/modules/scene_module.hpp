#pragma once

#include <memory>
#include <vector>

#include "tomop/tomop.hpp"

#include "../window.hpp"

namespace gui {

// for the 'one-way-communication' we have two parts
// a handler that knows how to read in a packet
// and an executor that knows how to execute a packet

class SceneModuleProtocol {

  public:

    SceneModuleProtocol() = default;
    virtual ~SceneModuleProtocol() = default;

    virtual std::unique_ptr<tomop::Packet> readPacket(
        tomop::PacketDesc desc,
        tomop::memory_buffer& buffer,
        zmq::socket_t& socket) = 0;

    virtual void process(MainWindow& window,
                         tomop::PacketDesc desc,
                         std::unique_ptr<tomop::Packet> event_packet) = 0;

    virtual std::vector<tomop::PacketDesc> descriptors() = 0;

    void ack(zmq::socket_t& socket) {
        zmq::message_t reply(sizeof(int));
        int success = 1;
        memcpy(reply.data(), &success, sizeof(int));
        socket.send(reply, zmq::send_flags::none);
    }
};

}  // namespace gui
