#pragma once

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"


namespace tomcat::recon {

class Server;

class ZmqServer {
    zmq::context_t context_;

    zmq::socket_t data_socket_;
    zmq::socket_t cmd_socket_;

    std::thread data_thread_;
    std::thread cmd_thread_;

    std::mutex send_mtx_;

    Server* server_;

  public:

    ZmqServer(int data_port, int message_port, Server* server);

    ~ZmqServer();

    void send(const tomcat::Packet& packet);

    void start();
};

} // tomcat::recon
