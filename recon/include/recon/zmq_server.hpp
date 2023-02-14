#ifndef SLICERECON_ZMQSERVER_H
#define SLICERECON_ZMQSERVER_H

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"


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

    void send(const tomcat::Packet& packet);

    void start();
};


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