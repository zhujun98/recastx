#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomop/tomop.hpp"

namespace tomovis {

using namespace tomop;

class Server {
    public:
        explicit Server(int port);

        void start();

    private:
        std::thread server_thread;

        zmq::context_t context_;
        zmq::socket_t rep_socket_;
        zmq::socket_t pub_socket_;

        void parse_packet(packet_desc desc);
};

}  // namespace tomovis
