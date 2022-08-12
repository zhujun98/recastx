#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomcat/tomcat.hpp"
#include "data_types.hpp"

namespace tomcat::consumer {

class Server {
    std::thread server_thread_;

    PacketQueue packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void parse_packet(PacketDesc desc);

  public:
    explicit Server(int port);

    void start();
};

}  // tomcat::consumer
