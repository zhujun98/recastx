#ifndef GUI_ZMQCLIENT_H
#define GUI_ZMQCLIENT_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include <zmq.hpp>

#include "logger.hpp"
#include "reconstruction.pb.h"

namespace recastx::gui {

class DataClient {

    std::thread thread_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    inline static std::queue<ReconDataPacket> packets_;

public:

    explicit DataClient(const std::string& hostname, int port);

    ~DataClient();

    static std::queue<ReconDataPacket>& packets();

    void start();
};

class MessageClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

public:

    explicit MessageClient(const std::string& hostname, int port);

    ~MessageClient();

    template<typename T>
    void send(T&& packet);
};

template<typename T>
void MessageClient::send(T&& packet) {
    try {
        std::string encoded;
        packet.SerializeToString(&encoded);

        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);

        log::debug("Published packet");
    } catch (const std::exception& e) {
        log::error("Failed publishing packet: {}", e.what());
    }
}

}  // namespace recastx::gui

#endif //GUI_ZMQCLIENT_H