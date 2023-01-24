#include <string>
#include <thread>

#include <zmq.hpp>

#include "server.hpp"

namespace tomcat::recon {

class DaqClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    std::shared_ptr<Server> server_;

public:
    DaqClient(const std::string& endpoint,
              zmq::socket_type socket_type,
              std::shared_ptr<Server> recon);

    ~DaqClient();

    void start();
};


} // tomcat::recon
