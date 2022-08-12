#include <chrono>
#include <iostream>
#include <thread>

#include <zmq.hpp>

#include "server.hpp"

namespace tomcat::consumer {

using namespace std::string_literals;

Server::Server(int port) : 
    rep_socket_(context_, ZMQ_REP), pub_socket_(context_, ZMQ_PUB) {
        rep_socket_.bind("tcp://*:"s + std::to_string(port));
        pub_socket_.bind("tcp://*:"s + std::to_string(port+1));
    }

void Server::parse_packet(PacketDesc desc) {
    switch (desc) {
        case PacketDesc::make_scene: {
            std::cout << "Received: make scene" << std::endl;
            break;
        }

        case PacketDesc::slice_data: {
            std::cout << "Received: slice data" << std::endl;
            break;
        }

        case PacketDesc::volume_data: {
            std::cout << "Received: volume data" << std::endl;
            break;
        }

        default: { 
            std::cout << "Received: unknown packet description!" << std::endl; 
        }
    }
}

void Server::start() {
    std::cout << "Listening for incoming connections..\n";

    // todo graceful shutdown, probably by sending a 'kill' packet to self
    server_thread_ = std::thread([&]() {
        while (true) {
            zmq::message_t request;

            rep_socket_.recv(request, zmq::recv_flags::none);
            auto desc = ((tomcat::PacketDesc*)request.data())[0];
            auto buffer = tomcat::memory_buffer(request.size(), (char*)request.data());

            // packets_->push({desc, std::move(modules_[desc]->read_packet(
            //     desc, buffer, rep_socket_, scenes_))});

            parse_packet(desc);

            std::string s = "OK";
            zmq::message_t message(s.size());
            memcpy(message.data(), s.data(), s.size());
            rep_socket_.send(message, zmq::send_flags::none);
        }
    });

    while (true) {
        if (packets_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //     // auto event_packet = std::move(packets_->front());
    //     // packets_->pop();

    //     // modules_[event_packet.first]->process(
    //     //     scenes_, event_packet.first, std::move(event_packet.second));
    }
}

} // tomcat::consumer
