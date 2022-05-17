#include <iostream>
#include <thread>
#include <type_traits>

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "modules/scene_module.hpp"
#include "scene.hpp"
#include "scene_list.hpp"
#include "server/server.hpp"

namespace tomovis {

using namespace tomop;
using namespace std::string_literals;

Server::Server(SceneList& scenes, int port)
    : scenes_(scenes),
      rep_socket_(context_, ZMQ_REP),
      pub_socket_(context_, ZMQ_PUB) {
    scenes_.add_listener(this);

    rep_socket_.bind("tcp://*:"s + std::to_string(port));
    pub_socket_.bind("tcp://*:"s + std::to_string(port+1));
}

void Server::register_module(std::shared_ptr<SceneModuleProtocol> module) {
    for (auto desc : module->descriptors()) {
        modules_[desc] = module;
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

            if (modules_.find(desc) == modules_.end()) {
                std::cout << "Unsupported package descriptor: "
                          << (std::underlying_type<decltype(desc)>::type)desc
                          << "\n";
                continue;
            }

            // forward the packet to the handler
            packets_.push({desc, std::move(modules_[desc]->read_packet(
                desc, buffer, rep_socket_, scenes_))});
        }
    });
}

void Server::tick(float) {
    while (!packets_.empty()) {
        auto event_packet = std::move(packets_.front());
        packets_.pop();

        modules_[event_packet.first]->process(
            scenes_, event_packet.first, std::move(event_packet.second));
    }
}

} // namespace tomovis
