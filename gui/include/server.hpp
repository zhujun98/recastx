#ifndef GUI_SERVER_H
#define GUI_SERVER_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class Server {

public:

    using DataType = std::pair<PacketDesc, std::unique_ptr<Packet>>;

private:

    std::thread thread_;
    bool running_ = false;

    inline static std::queue<DataType> packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void ack();

  public:

    explicit Server(int port);
    ~Server();

    void send(Packet& packet);

    void start();

    std::queue<DataType>& packets();
};

}  // namespace tomcat::gui

#endif //GUI_SERVER_H