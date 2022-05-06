#pragma once

#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <zmq.hpp>

#include "tomop/tomop.hpp"

namespace slicerecon {

class plugin {
  public:
    using callback_type =
        std::function<std::pair<std::array<int32_t, 2>, std::vector<float>>(
            std::array<int32_t, 2>, std::vector<float>, int32_t)>;

    plugin(std::string hostname_in = "tcp://*:5650",
           std::string hostname_out = "tcp://localhost:5555")
        : context_(1), socket_in_(context_, ZMQ_REP),
          socket_out_(context_, ZMQ_REQ) {
        spdlog::info("Plugin: {} -> {}", hostname_in, hostname_out);

        socket_in_.bind(hostname_in);
        socket_out_.connect(hostname_out);
    }

    ~plugin() { serve_thread_.join(); }

    void serve() {
        serve_thread_ = std::thread([&] { listen(); });

        serve_thread_.join();
    }

    void ack() {
        zmq::message_t reply(sizeof(int));
        int success = 1;
        memcpy(reply.data(), &success, sizeof(int));
        socket_in_.send(reply, zmq::send_flags::none);
    }

    void send(const tomop::Packet& packet) {
        packet.send(socket_out_);
        zmq::message_t reply;
        socket_out_.recv(reply, zmq::recv_flags::none);
    }

    void set_slice_callback(callback_type callback) {
        slice_data_callback_ = callback;
    }

    void listen() {
        spdlog::info("Plugin starts listening.");

        while (true) {
            zmq::message_t update;
            bool kill = false;
            if (!socket_in_.recv(update, zmq::recv_flags::none)) {
                kill = true;
            } else {
                ack();

                auto desc = ((tomop::packet_desc*)update.data())[0];
                auto buffer =
                    tomop::memory_buffer(update.size(), (char*)update.data());

                switch (desc) {
                case tomop::packet_desc::slice_data: {
                    auto packet = std::make_unique<tomop::SliceDataPacket>();
                    packet->deserialize(std::move(buffer));

                    if (!slice_data_callback_) {
                        throw tomop::server_error("No callback set for plugin");
                    }

                    auto callback_data = slice_data_callback_(
                        packet->slice_size, std::move(packet->data),
                        packet->slice_id);

                    packet->slice_size = std::get<0>(callback_data);
                    packet->data = std::move(std::get<1>(callback_data));

                    send(*packet);
                    break;
                }
                case tomop::packet_desc::remove_slice: {
                    std::cout << "Plugin: removing slice TODO pass along to "
                                 "Python interface callback\n";
                    break;
                }
                case tomop::packet_desc::kill_scene: {
                    auto packet = std::make_unique<tomop::KillScenePacket>();
                    packet->deserialize(std::move(buffer));

                    kill = true;

                    // pass it along
                    send(*packet);

                    break;
                }
                    // TODO add support for registered parameters
                default:
                    break;
                }
            }

            if (kill) {
                std::cout << "Scene closed...\n";
                break;
            }
        }
    }

  private:
    zmq::context_t context_;
    zmq::socket_t socket_in_;
    zmq::socket_t socket_out_;
    std::thread serve_thread_;

    callback_type slice_data_callback_;
};

} // namespace slicerecon
