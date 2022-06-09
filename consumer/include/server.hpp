#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomop/tomop.hpp"
#include "data_types.hpp"

namespace tomovis {

using namespace tomop;

class Server {
    std::thread server_thread_;

    PacketQueue packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void parse_packet(packet_desc desc);

  public:
    explicit Server(int port);

    void start();
};

}  // namespace tomovis
