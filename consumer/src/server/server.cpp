#include <iostream>
#include <thread>

#include <zmq.hpp>

#include "tomop/tomop.hpp"

#include "server/server.hpp"

namespace tomovis {

using namespace tomop;
using namespace std::string_literals;

Server::Server(int port) : 
    rep_socket_(context_, ZMQ_REP), pub_socket_(context_, ZMQ_PUB) {
        rep_socket_.bind("tcp://*:"s + std::to_string(port));
        pub_socket_.bind("tcp://*:"s + std::to_string(port+1));
    }

void Server::parse_packet(packet_desc desc) {
    switch (desc) {
        case packet_desc::slice_data: {
            std::cout << "Received: slice data" << std::endl;
            break;
        }

        case packet_desc::partial_slice_data: {
            std::cout << "Received: partial slice data" << std::endl;
            break;
        }

        case packet_desc::volume_data: {
            std::cout << "Received: volume data" << std::endl;
            break;
        }

        case packet_desc::partial_volume_data: {
            std::cout << "Received: partial volume data" << std::endl;
            break;
        }

        case packet_desc::group_request_slices: {
            std::cout << "Received: group request slices" << std::endl;
            break;
        }

        default: { 
            std::cout << "Received: unknown packet description!" << std::endl; 
        }
    }
}

void Server::start() {
    //  Prepare our context and socket
    std::cout << "Listening for incoming connections..\n";

    // todo graceful shutdown, probably by sending a 'kill' packet to self
    server_thread = std::thread([&]() {

        while (true) {
            zmq::message_t request;

            //  Wait for next request from client
            rep_socket_.recv(request, zmq::recv_flags::none);
            auto desc = ((packet_desc*)request.data())[0];
            auto buffer = memory_buffer(request.size(), (char*)request.data());

            parse_packet(desc);

            std::string s = "OK";
            zmq::message_t message(s.size());
            memcpy(message.data(), s.data(), s.size());
            rep_socket_.send(message, zmq::send_flags::none);
        }
    });
}

} // namespace tomovis
