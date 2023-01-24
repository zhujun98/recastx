#include <string>
#include <thread>

#include <zmq.hpp>

namespace tomcat::recon {

class Server;

class DaqClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    Server* server_;

    zmq::socket_type parseSocketType(const std::string& socket_type) const; 

public:
    DaqClient(const std::string& endpoint,
              const std::string& socket_type,
              Server* server);

    ~DaqClient();

    void start();
};


} // tomcat::recon
