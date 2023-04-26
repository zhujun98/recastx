#ifndef RECON_DAQCLIENT_H
#define RECON_DAQCLIENT_H

#include <string>
#include <thread>

#include <zmq.hpp>

#include "state.pb.h"

namespace recastx::recon {

class Application;

class DaqClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    Application* app_;

    zmq::socket_type parseSocketType(const std::string& socket_type) const; 

    bool initialized_ = false;
    size_t num_rows_;
    size_t num_cols_;

    StatePacket_State state_;

public:

    DaqClient(const std::string& endpoint,
              const std::string& socket_type,
              Application* app);

    ~DaqClient();

    void start();

    void setState(StatePacket_State state);
};


} // namespace recastx::recon

#endif // RECON_DAQCLIENT_H