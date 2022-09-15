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

class Client {

public:

    using DataType = std::pair<PacketDesc, std::unique_ptr<Packet>>;

private:

    std::thread thread_;

    inline static std::queue<DataType> packets_;

    zmq::context_t context_;
    zmq::socket_t data_socket_;
    zmq::socket_t cmd_socket_;

  public:

    explicit Client(const std::string& hostname, int port);
    ~Client();

    void send(const Packet& packet);

    void start();

    std::queue<DataType>& packets();
};

}  // namespace tomcat::gui

#endif //GUI_SERVER_H