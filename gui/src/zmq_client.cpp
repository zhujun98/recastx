#include <thread>
#include <type_traits>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "zmq_client.hpp"
#include "reconstruction.pb.h"
#include "tomcat/tomcat.hpp"


namespace tomcat::gui {

using namespace std::string_literals;

DataClient::DataClient(const std::string& hostname, int port)
      : context_(1), socket_(context_, ZMQ_REQ) {

    std::string endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    socket_.connect(endpoint);

    spdlog::info("Connecting to the reconstruction data server: {}", endpoint);
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

            ReconDataPacket packet;
            packet.ParseFromArray(reply.data(), static_cast<int>(reply.size()));
            packets_.push(std::move(packet));
        }
    });

    thread_.detach();
}

std::queue<ReconDataPacket>& DataClient::packets() { return packets_; }


MessageClient::MessageClient(const std::string &hostname, int port)
        : context_(1), socket_(context_, ZMQ_PAIR) {

    std::string endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    socket_.connect(endpoint);

    spdlog::info("Connecting to the reconstruction message server: {}", endpoint);
}

MessageClient::~MessageClient() {
    socket_.set(zmq::sockopt::linger, 200);
};

void MessageClient::send(const ReconRequestPacket& packet) {
    try {
        std::string encoded;
        packet.SerializeToString(&encoded);

        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);

        spdlog::debug("Published packet");

    } catch (const std::exception& e) {
        spdlog::error("Failed publishing packet: {}", e.what());
    }
}

} // namespace tomcat::gui
