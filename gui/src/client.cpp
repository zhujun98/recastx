#include <thread>
#include <type_traits>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "client.hpp"
#include "tomcat/tomcat.hpp"


namespace tomcat::gui {

using namespace std::string_literals;

Client::Client(const std::string& hostname, int port)
      : context_(1),
        data_socket_(context_, ZMQ_REQ),
        cmd_socket_(context_, ZMQ_PAIR) {

    // Caveat: sequence
    std::string cmd_endpoint = "tcp://"s + hostname + ":"s + std::to_string(port + 1);
    cmd_socket_.connect(cmd_endpoint);

    std::string data_endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    data_socket_.connect(data_endpoint);

    spdlog::info("Connecting to the reconstruction server: {}, {}",
                 data_endpoint, cmd_endpoint);
}

Client::~Client() {
    data_socket_.set(zmq::sockopt::linger, 200);
};

void Client::start() {
    thread_ = std::thread([&]() {
        while (true) {
            data_socket_.send(zmq::str_buffer("ready"), zmq::send_flags::none);

            zmq::message_t reply;
            data_socket_.recv(reply, zmq::recv_flags::none);
            auto desc = ((PacketDesc*)reply.data())[0];
            auto buffer = tomcat::memory_buffer(reply.size(), (char*)reply.data());
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
        }
    });

    thread_.detach();
}

std::queue<Client::DataType>& Client::packets() { return packets_; }

void Client::send(const Packet& packet) {
    try {
        auto size = packet.size();
        zmq::message_t message(size);
        auto membuf = packet.serialize(size);
        memcpy(message.data(), membuf.buffer.get(), size);
        cmd_socket_.send(message, zmq::send_flags::none);

#if (VERBOSITY >= 3)
        spdlog::info("Published packet: 0x{0:x}",
                     std::underlying_type<PacketDesc>::type(packet.desc()));
#endif

    } catch (const std::exception& e) {
        spdlog::error("Failed publishing packet: {}", e.what());
    }
}

} // namespace tomcat::gui
