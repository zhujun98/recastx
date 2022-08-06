#include <iostream>
#include <thread>
#include <type_traits>

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"
#include "modules/reconstruction.hpp"
#include "scene.hpp"
#include "server.hpp"


namespace gui {

using namespace std::string_literals;

Server::Server(SceneList& scenes, int port)
    : scenes_(scenes),
      rep_socket_(context_, ZMQ_REP),
      pub_socket_(context_, ZMQ_PUB) {
    scenes_.addPublisher(this);

    rep_socket_.bind("tcp://*:"s + std::to_string(port));
    pub_socket_.bind("tcp://*:"s + std::to_string(port+1));

    registerModule(std::make_shared<ReconstructionProtocol>());
}

void Server::start() {
    std::cout << "Listening for incoming connections..\n";

    thread_ = std::thread([&]() {
        while (true) {
            zmq::message_t request;

            rep_socket_.recv(request, zmq::recv_flags::none);
            auto desc = ((tomop::PacketDesc*)request.data())[0];
            auto buffer = tomop::memory_buffer(request.size(), (char*)request.data());

            if (modules_.find(desc) == modules_.end()) {
                spdlog::warn("Unsupported package descriptor: 0x{0:x}",
                             std::underlying_type<tomop::PacketDesc>::type(desc));
                continue;
            }

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

void Server::sendPacket(tomop::Packet& packet) {
    try {
        auto size = packet.size();
        zmq::message_t message(size);
        auto membuf = packet.serialize(size);
        memcpy(message.data(), membuf.buffer.get(), size);
        pub_socket_.send(message, zmq::send_flags::none);

#if (VERBOSITY >= 3)
        spdlog::info("Published packet: 0x{0:x}", 
                     std::underlying_type<tomop::PacketDesc>::type(packet.desc()));
#endif

    } catch (const std::exception& e) {
        spdlog::error("Failed publishing packet: {}", e.what());
    }
}

void Server::registerModule(const std::shared_ptr<SceneModuleProtocol>& module) {
    for (auto desc : module->descriptors()) modules_[desc] = module;
}

} // namespace gui
