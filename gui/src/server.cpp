#include <iostream>
#include <thread>
#include <type_traits>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "server.hpp"
#include "tomcat/tomcat.hpp"


namespace tomcat::gui {

using namespace std::string_literals;

Server::Server(int port)
      : rep_socket_(context_, ZMQ_REP),
        pub_socket_(context_, ZMQ_PUB) {

    rep_socket_.bind("tcp://*:"s + std::to_string(port));
    pub_socket_.bind("tcp://*:"s + std::to_string(port+1));
}

Server::~Server() = default;

void Server::start() {
    std::cout << "Listening for incoming connections..\n";

    running_ = true;
    thread_ = std::thread([&]() {
        while (running_) {
            zmq::message_t request;

            rep_socket_.recv(request, zmq::recv_flags::none);
            auto desc = ((PacketDesc*)request.data())[0];
            auto buffer = tomcat::memory_buffer(request.size(), (char*)request.data());

            switch (desc) {
                case PacketDesc::slice_data: {
                    auto packet = std::make_unique<SliceDataPacket>();
                    packet->deserialize(std::move(buffer));
                    packets_.push({desc, std::move(packet)});
                    break;
                }
                case PacketDesc::volume_data: {
                    auto packet = std::make_unique<VolumeDataPacket>();
                    packet->deserialize(std::move(buffer));
                    packets_.push({desc, std::move(packet)});
                    break;
                }
                default: {
                    spdlog::warn("Unknown package descriptor: 0x{0:x}",
                                 std::underlying_type<PacketDesc>::type(desc));
                    break;
                }
            }

            ack();
        }
    });
}

std::queue<Server::DataType>& Server::packets() { return packets_; }

void Server::send(Packet& packet) {
    try {
        auto size = packet.size();
        zmq::message_t message(size);
        auto membuf = packet.serialize(size);
        memcpy(message.data(), membuf.buffer.get(), size);
        pub_socket_.send(message, zmq::send_flags::none);

#if (VERBOSITY >= 3)
        spdlog::info("Published packet: 0x{0:x}", 
                     std::underlying_type<PacketDesc>::type(packet.desc()));
#endif

    } catch (const std::exception& e) {
        spdlog::error("Failed publishing packet: {}", e.what());
    }
}

void Server::ack() {
    zmq::message_t reply(sizeof(int));
    int success = 1;
    memcpy(reply.data(), &success, sizeof(int));
    rep_socket_.send(reply, zmq::send_flags::none);
}

} // namespace tomcat::gui
