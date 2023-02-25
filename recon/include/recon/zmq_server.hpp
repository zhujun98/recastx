#ifndef SLICERECON_ZMQSERVER_H
#define SLICERECON_ZMQSERVER_H

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>


namespace tomcat::recon {

class Application;

class DataServer {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    std::mutex send_mtx_;

    Application* app_;

public:

    DataServer(int port, Application* app);

    ~DataServer();

    template<typename T>
    void send(const T& data);

    void start();
};

template<typename T>
void DataServer::send(const T& packet) {
    std::string encoded;
    packet.SerializeToString(&encoded);

    zmq::message_t msg;
    socket_.recv(msg, zmq::recv_flags::none);
    auto request = std::string(static_cast<char*>(msg.data()), msg.size());
    if (request == "ready") {
        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);
    } else {
        spdlog::warn("Unknown request received: {}", request);
    }
}


class MessageServer {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    Application* app_;

public:

    MessageServer(int port, Application* app);

    ~MessageServer();

    void start();
};


} // tomcat::recon

#endif // SLICERECON_ZMQSERVER_H