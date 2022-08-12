#pragma once

#include <array>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"
#include "reconstructor.hpp"

namespace tomcat::recon {

class Broker {
    zmq::context_t context_;

    zmq::socket_t req_socket_;
    zmq::socket_t sub_socket_;

    std::thread req_thread_;
    std::thread sub_thread_;

    std::shared_ptr<Reconstructor> recon_;

  public:

    Broker(const std::string& endpoint,
           const std::string& subscribe_endpoint,
           std::shared_ptr<Reconstructor> recon);

    ~Broker();

    void send(const tomcat::Packet& packet);

    void start();
};

} // tomcat::recon
