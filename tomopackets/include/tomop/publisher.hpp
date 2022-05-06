#pragma once

#include <cstddef>
#include <string>

#include <zmq.hpp>

#include "exceptions.hpp"
#include "packets.hpp"

namespace tomop {

class publisher {
  public:
    publisher(std::string hostname = "localhost", int32_t port = 5555,
              int32_t type = ZMQ_PUSH)
        : context_(1), socket_(context_, type), type_(type) {
        using namespace std::string_literals;
        using namespace std::chrono_literals;

        address_ = "tcp://"s + hostname + ":"s + std::to_string(port);
        socket_.set(zmq::sockopt::linger, 200);
        socket_.connect(address_);
    }

    ~publisher() {
        socket_.close();
        context_.close();
    }

    void send(const Packet& packet) {
        packet.send(socket_);

        if (type_ == ZMQ_REQ) {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
        }
    }

  private:
    // publisher connection
    zmq::context_t context_;
    zmq::socket_t socket_;

    int type_;
    std::string address_;
};

} // namespace tomop
