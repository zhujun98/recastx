#include <string>
#include <thread>

#include <zmq.hpp>

namespace tomcat::recon {

class Application;

class DaqClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    Application* app_;

    zmq::socket_type parseSocketType(const std::string& socket_type) const; 

public:
    DaqClient(const std::string& endpoint,
              const std::string& socket_type,
              Application* app);

    ~DaqClient();

    void start();
};


} // tomcat::recon
