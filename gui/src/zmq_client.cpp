#include <thread>
#include <type_traits>

#include <zmq.hpp>

#include "zmq_client.hpp"
#include "logger.hpp"

namespace recastx::gui {

using namespace std::string_literals;

DataClient::DataClient(const std::string& hostname, int port)
      : context_(1), socket_(context_, ZMQ_REQ) {

    std::string endpoint = "tcp://"s + hostname + ":"s + std::to_string(port);
    socket_.connect(endpoint);

    log::info("Connecting to the reconstruction data server: {}", endpoint);
}

DataClient::~DataClient() {
    socket_.set(zmq::sockopt::linger, 200);
    context_.shutdown();
};

void DataClient::start() {
    thread_ = std::thread([&]() {
        while (true) {
            zmq::message_t reply;

            try {
                socket_.send(zmq::str_buffer("GUIReady"), zmq::send_flags::none);
                [[maybe_unused]] auto recv_ret = socket_.recv(reply, zmq::recv_flags::none);
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

    log::info("Connecting to the reconstruction message server: {}", endpoint);
}

MessageClient::~MessageClient() {
    socket_.set(zmq::sockopt::linger, 200);
    context_.shutdown();
};

void MessageClient::send(const ReconRequestPacket& packet) {
    try {
        std::string encoded;
        packet.SerializeToString(&encoded);

        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);

        log::debug("Published packet");

    } catch (const std::exception& e) {
        log::error("Failed publishing packet: {}", e.what());
    }
}

} // namespace recastx::gui
