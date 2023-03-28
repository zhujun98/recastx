#ifndef GUI_ZMQCLIENT_H
#define GUI_ZMQCLIENT_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include <zmq.hpp>

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

    void send(const ReconRequestPacket& packet);
};

}  // namespace recastx::gui

#endif //GUI_ZMQCLIENT_H