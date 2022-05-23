#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"
#include <zmq.hpp>

#include <mutex>

#include "reconstruction/listener.hpp"
#include "reconstruction/reconstructor.hpp"
#include "util/bench.hpp"
#include "data_types.hpp"

namespace slicerecon {

class VisualizationServer : public Listener, public util::bench_listener {

public:

    using callback_type =
        std::function<slice_data(std::array<float, 9>, int32_t)>;

private:

    // server connection
    zmq::context_t context_;
    zmq::socket_t socket_;

    // subscribe connection
    std::thread serve_thread_;
    zmq::socket_t subscribe_socket_;

    // subscribe connection
    std::optional<zmq::socket_t> plugin_socket_;

    int32_t scene_id_ = -1;

    callback_type slice_data_callback_;
    std::vector<std::pair<int32_t, std::array<float, 9>>> slices_;

    std::mutex socket_mutex_;

public:

    void notify(Reconstructor& recon) override {

        spdlog::info("Sending volume preview ...");

        zmq::message_t reply;
        int n = recon.parameters().preview_size;

        auto volprev = tomop::VolumeDataPacket(scene_id_, {n, n, n}, recon.previewData());
        send(volprev);

        auto grsp = tomop::GroupRequestSlicesPacket(scene_id_, 1);
        send(grsp);
    }

    void bench_notify(std::string name, double time) override {
        send(tomop::BenchmarkPacket(scene_id_, name, time));
    }

    void register_parameter(
        std::string parameter_name,
        std::variant<float, std::vector<std::string>, bool> value) override {
        // make parameter packet and send it
        if (std::holds_alternative<float>(value)) {
            auto x = std::get<float>(value);
            send(tomop::ParameterFloatPacket(scene_id_, parameter_name, x));
        }
        if (std::holds_alternative<std::vector<std::string>>(value)) {
            auto xs = std::get<std::vector<std::string>>(value);
            send(tomop::ParameterEnumPacket(scene_id_, parameter_name, xs));
        }
        if (std::holds_alternative<bool>(value)) {
            auto x = std::get<bool>(value);
            send(tomop::ParameterBoolPacket(scene_id_, parameter_name, x));
        }
    }

    VisualizationServer(const std::string& name,
                        const std::string& hostname,
                        const std::string& subscribe_hostname)
        : context_(1), 
          socket_(context_, ZMQ_REQ),
          subscribe_socket_(context_, ZMQ_SUB) {
        using namespace std::chrono_literals;

        // set socket timeout to 200 ms
        socket_.set(zmq::sockopt::linger, 200);
        socket_.connect(hostname);

        auto packet = tomop::MakeScenePacket(name, 3);

        packet.send(socket_);

        // check if we get a reply within a second
        zmq::pollitem_t items[] = {{socket_, 0, ZMQ_POLLIN, 0}};
        auto poll_result = zmq::poll(items, 1, 1000ms);

        if (poll_result <= 0) {
            throw tomop::server_error("Could not connect to server: " + hostname);
        } else {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
            scene_id_ = *(int32_t*)reply.data();
            std::cout << scene_id_ << std::endl;
            // std::cout << std::string(static_cast<char*>(reply.data()), reply.size()) << std::endl;
        }

        subscribe(subscribe_hostname);

        spdlog::info("Connected to visualization server: {}", hostname);
    }

    void register_plugin(std::string plugin_hostname) {
        plugin_socket_ = zmq::socket_t(context_, ZMQ_REQ);
        plugin_socket_.value().connect(plugin_hostname);
    }

    ~VisualizationServer() {
        if (serve_thread_.joinable()) {
            serve_thread_.join();
        }

        socket_.close();
        subscribe_socket_.close();
        context_.close();
    }

    void send(const tomop::Packet& packet, bool try_plugin = false) {
        std::lock_guard<std::mutex> guard(socket_mutex_);

        zmq::message_t reply;

        if (try_plugin && plugin_socket_) {
            packet.send(plugin_socket_.value());
            plugin_socket_.value().recv(reply, zmq::recv_flags::none);
        } else {
            packet.send(socket_);
            socket_.recv(reply, zmq::recv_flags::none);
        }
    }

    void subscribe(std::string subscribe_host) {
        if (scene_id_ < 0) {
            throw tomop::server_error("Subscribe called for uninitialized server");
        }

        // set socket timeout to 200 ms
        socket_.set(zmq::sockopt::linger, 200);

        //  Socket to talk to server
        subscribe_socket_.connect(subscribe_host);

        std::vector<tomop::packet_desc> descriptors = {
            tomop::packet_desc::set_slice,
            tomop::packet_desc::remove_slice,
            tomop::packet_desc::kill_scene,
            tomop::packet_desc::parameter_float,
            tomop::packet_desc::parameter_bool,
            tomop::packet_desc::parameter_enum
        };

        for (auto descriptor : descriptors) {
            int32_t filter[] = {
                (std::underlying_type<tomop::packet_desc>::type)descriptor, scene_id_
            };
            subscribe_socket_.set(zmq::sockopt::subscribe, zmq::buffer(filter));
        }
    }

    void serve() {
        serve_thread_ = std::thread([&] {
            while (true) {
                zmq::message_t update;
                bool kill = false;
                if (!subscribe_socket_.recv(update, zmq::recv_flags::none)) {
                    kill = true;
                } else {
                    auto desc = ((tomop::packet_desc*)update.data())[0];
                    auto buffer = tomop::memory_buffer(update.size(), (char*)update.data());

                    switch (desc) {
                        case tomop::packet_desc::kill_scene: {
                            auto packet = std::make_unique<tomop::KillScenePacket>();
                            packet->deserialize(std::move(buffer));

                            if (packet->scene_id != scene_id_) {
                                spdlog::warn("Received kill request with wrong scene id!");
                            } else {
                                kill = true;
                            }
                            break;
                        }
                        case tomop::packet_desc::set_slice: {
                            auto packet = std::make_unique<tomop::SetSlicePacket>();
                            packet->deserialize(std::move(buffer));

                            make_slice(packet->slice_id, packet->orientation);
                            break;
                        }
                        case tomop::packet_desc::remove_slice: {
                            auto packet = std::make_unique<tomop::RemoveSlicePacket>();
                            packet->deserialize(std::move(buffer));

                            auto to_erase = std::find_if(
                                slices_.begin(), slices_.end(), [&](auto x) {
                                    return x.first == packet->slice_id;
                                });
                            slices_.erase(to_erase);

                            if (plugin_socket_) send(*packet, true);

                            break;
                        }
                        case tomop::packet_desc::parameter_float: {
                            auto packet = std::make_unique<tomop::ParameterFloatPacket>();
                            packet->deserialize(std::move(buffer));
                            parameter_changed(packet->parameter_name, {packet->value});

                            if (plugin_socket_) send(*packet, true);

                            break;
                        }
                        case tomop::packet_desc::parameter_enum: {
                            auto packet = std::make_unique<tomop::ParameterEnumPacket>();
                            packet->deserialize(std::move(buffer));
                            parameter_changed(packet->parameter_name, {packet->values[0]});

                            if (plugin_socket_) send(*packet, true);

                            break;
                        }
                        case tomop::packet_desc::parameter_bool: {
                            auto packet = std::make_unique<tomop::ParameterBoolPacket>();
                            packet->deserialize(std::move(buffer));
                            parameter_changed(packet->parameter_name, {packet->value});

                            if (plugin_socket_) send(*packet, true);

                            break;
                        }
                        default:
                            spdlog::warn("Unrecognized package with descriptor {0:x}", 
                                         std::underlying_type<tomop::packet_desc>::type(desc));
                            break;
                        }
                }

                if (kill) {
                    spdlog::info("Scene closed.");
                    break;
                }
            }
        });

        serve_thread_.join();
    }

    void make_slice(int32_t slice_id, std::array<float, 9> orientation) {
        int32_t update_slice_index = -1;
        int32_t i = 0;
        for (auto& id_and_slice : slices_) {
            if (id_and_slice.first == slice_id) {
                update_slice_index = i;
                break;
            }
            ++i;
        }

        if (update_slice_index >= 0) {
            slices_[update_slice_index] = std::make_pair(slice_id, orientation);
        } else {
            slices_.push_back(std::make_pair(slice_id, orientation));
        }

        if (!slice_data_callback_) {
            throw tomop::server_error("No callback set");
        }

        auto result = slice_data_callback_(orientation, slice_id);

        if (!result.first.empty()) {
            auto data_packet = tomop::SliceDataPacket(
                scene_id_, slice_id, result.first, std::move(result.second), false);
            send(data_packet, true);
        }
    }

    void set_slice_callback(callback_type callback) {
        slice_data_callback_ = callback;
    }

    int32_t scene_id() const { return scene_id_; }

};

} // namespace slicerecon
