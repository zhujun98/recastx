#include <thread>
#include <type_traits>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "client.hpp"
#include "tomcat/tomcat.hpp"


namespace tomcat::gui {

using namespace std::string_literals;

DataClient::DataClient(const std::string& hostname, int port)
      : context_(1), socket_(context_, ZMQ_REQ) {

    std::string endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    socket_.connect(endpoint);

    spdlog::info("Connecting to the reconstruction server: {}", endpoint);
}

DataClient::~DataClient() {
    socket_.set(zmq::sockopt::linger, 200);
};

void DataClient::start() {
    thread_ = std::thread([&]() {
        while (true) {
            zmq::message_t reply;

            try {
                socket_.send(zmq::str_buffer("ready"), zmq::send_flags::none);
                auto recv_ret = socket_.recv(reply, zmq::recv_flags::none);
            } catch (const zmq::error_t& e) {
                if (e.num() != ETERM) throw;
                break;
            }

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

std::queue<PacketDataEvent>& DataClient::packets() { return packets_; }


CmdClient::CmdClient(const std::string &hostname, int port)
        : context_(1), socket_(context_, ZMQ_PAIR) {

    std::string endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    socket_.connect(endpoint);

    spdlog::info("Connecting to the reconstruction server: {}", endpoint);
}

CmdClient::~CmdClient() {
    socket_.set(zmq::sockopt::linger, 200);
};

void CmdClient::send(const Packet& packet) {
    try {
        auto size = packet.size();
        zmq::message_t message(size);
        auto membuf = packet.serialize(size);
        memcpy(message.data(), membuf.buffer.get(), size);
        socket_.send(message, zmq::send_flags::none);

        spdlog::debug("Published packet: 0x{0:x}",
                      std::underlying_type<PacketDesc>::type(packet.desc()));

    } catch (const std::exception& e) {
        spdlog::error("Failed publishing packet: {}", e.what());
    }
}

} // namespace tomcat::gui
