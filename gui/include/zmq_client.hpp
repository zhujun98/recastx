#ifndef GUI_ZMQCLIENT_H
#define GUI_ZMQCLIENT_H

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

class MessageClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

public:

    explicit MessageClient(const std::string& hostname, int port);

    ~MessageClient();

    void send(const Packet& packet);
};

}  // namespace tomcat::gui

#endif //GUI_ZMQCLIENT_H