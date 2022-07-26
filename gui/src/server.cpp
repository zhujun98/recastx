#include <iostream>
#include <thread>
#include <type_traits>

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "modules/reconstruction.hpp"
#include "modules/control.hpp"
#include "scene.hpp"
#include "server.hpp"


namespace gui {

using namespace std::string_literals;

Server::Server(SceneList& scenes, int port)
    : scenes_(scenes),
      rep_socket_(context_, ZMQ_REP),
      pub_socket_(context_, ZMQ_PUB) {
    scenes_.add_listener(this);

    rep_socket_.bind("tcp://*:"s + std::to_string(port));
    pub_socket_.bind("tcp://*:"s + std::to_string(port+1));

    registerModule(std::make_shared<ReconstructionProtocol>());
    registerModule(std::make_shared<ControlProtocol>());
}

void Server::start() {
    std::cout << "Listening for incoming connections..\n";

    thread_ = std::thread([&]() {
        while (true) {
            zmq::message_t request;

            //  Wait for next request from client
            rep_socket_.recv(request, zmq::recv_flags::none);
            auto desc = ((tomop::PacketDesc*)request.data())[0];
            auto buffer = tomop::memory_buffer(request.size(), (char*)request.data());

            if (modules_.find(desc) == modules_.end()) {
                std::cout << "Unsupported package descriptor: "
                          << std::hex << (std::underlying_type<decltype(desc)>::type)desc
                          << "\n";
                continue;
            }

            // forward the packet to the handler
            packets_.push({desc, std::move(modules_[desc]->readPacket(
                desc, buffer, rep_socket_))});
        }
    });
}

void Server::tick(float) {
    while (!packets_.empty()) {
        auto packet = std::move(packets_.front());
        packets_.pop();

        modules_[packet.first]->process(
            scenes_, packet.first, std::move(packet.second));
    }
}

void Server::handle(tomop::Packet& pkt) {
    try {
        auto pkt_size = pkt.size();
        zmq::message_t message(pkt_size);
        auto membuf = pkt.serialize(pkt_size);
        memcpy(message.data(), membuf.buffer.get(), pkt_size);
        pub_socket_.send(message, zmq::send_flags::none);
    } catch (const std::exception& e) {
        std::cout << "Failed sending: " << e.what() << "\n";
    }
}

void Server::registerModule(const std::shared_ptr<SceneModuleProtocol>& module) {
    for (auto desc : module->descriptors()) modules_[desc] = module;
}

} // namespace gui
