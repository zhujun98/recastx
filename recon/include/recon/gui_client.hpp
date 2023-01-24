#pragma once

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"
#include "server.hpp"

namespace tomcat::recon {

class GuiClient {
    zmq::context_t context_;

    zmq::socket_t data_socket_;
    zmq::socket_t cmd_socket_;

    std::thread data_thread_;
    std::thread cmd_thread_;

    std::mutex send_mtx_;

    std::shared_ptr<Server> server_;

  public:

    GuiClient(int gui_port, std::shared_ptr<Server> server);

    ~GuiClient();

    void send(const tomcat::Packet& packet);

    void start();
};

} // tomcat::recon
