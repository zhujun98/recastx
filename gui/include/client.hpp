#ifndef GUI_CLIENT_H
#define GUI_CLIENT_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class DataClient {

    std::thread thread_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    inline static std::queue<PacketDataEvent> packets_;

public:

    explicit DataClient(const std::string& hostname, int port);

    ~DataClient();

    static std::queue<PacketDataEvent>& packets();

    void start();
};

class CmdClient {
    zmq::context_t context_;
    zmq::socket_t socket_;

public:

    explicit CmdClient(const std::string& hostname, int port);

    ~CmdClient();

    void send(const Packet& packet);
};

}  // namespace tomcat::gui

#endif //GUI_CLIENT_H