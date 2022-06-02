#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>
#include <zmq.hpp>

#include "tomop/tomop.hpp"
#include "reconstructor.hpp"
#include "data_types.hpp"

namespace slicerecon {

class VisualizationServer {
    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;
    std::thread preview_thread_;
    zmq::socket_t sub_socket_;

    int32_t scene_id_ = -1;

    std::vector<std::pair<int32_t, std::array<float, 9>>> slices_;

    std::mutex socket_mutex_;

    std::shared_ptr<Reconstructor> recon_;

public:

    VisualizationServer(const std::string& name,
                        const std::string& endpoint,
                        const std::string& subscribe_endpoint,
                        std::shared_ptr<Reconstructor> recon)
        : context_(1), 
          socket_(context_, ZMQ_REQ),
          sub_socket_(context_, ZMQ_SUB),
          recon_(recon) {
        using namespace std::chrono_literals;

        socket_.set(zmq::sockopt::linger, 200);
        socket_.connect(endpoint);

        auto packet = tomop::MakeScenePacket(name, 3);

        packet.send(socket_);

        // check if we get a reply within a second
        zmq::pollitem_t items[] = {{socket_, 0, ZMQ_POLLIN, 0}};
        auto poll_result = zmq::poll(items, 1, 1000ms);

        if (poll_result <= 0) {
            throw std::runtime_error("Could not connect to server: " + endpoint);
        } else {
            zmq::message_t reply;
            socket_.recv(reply, zmq::recv_flags::none);
            scene_id_ = *(int32_t*)reply.data();
            // std::cout << std::string(static_cast<char*>(reply.data()), reply.size()) << std::endl;
        }

        subscribe(subscribe_endpoint);

        spdlog::info("Connected to visualization server: {}", endpoint);
    }

    ~VisualizationServer() {
        socket_.close();
        sub_socket_.close();
        context_.close();
    }

    void send(const tomop::Packet& packet) {
        std::lock_guard<std::mutex> guard(socket_mutex_);

        zmq::message_t reply;
        packet.send(socket_);
        socket_.recv(reply, zmq::recv_flags::none);
    }

    void subscribe(std::string subscribe_host) {
        if (scene_id_ < 0) {
            throw std::runtime_error("Subscribe called for uninitialized server");
        }

        // set socket timeout to 200 ms
        socket_.set(zmq::sockopt::linger, 200);

        //  Socket to talk to server
        sub_socket_.connect(subscribe_host);

        std::vector<tomop::packet_desc> descriptors = {
            tomop::packet_desc::set_slice,
            tomop::packet_desc::remove_slice
        };

        for (auto descriptor : descriptors) {
            int32_t filter[] = {
                (std::underlying_type<tomop::packet_desc>::type)descriptor, scene_id_
            };
            sub_socket_.set(zmq::sockopt::subscribe, zmq::buffer(filter));
        }
    }

    void start() {
        thread_ = std::thread([&] {
            while (true) {
                zmq::message_t update;
                sub_socket_.recv(update, zmq::recv_flags::none);
                auto desc = ((tomop::packet_desc*)update.data())[0];
                auto buffer = tomop::memory_buffer(update.size(), (char*)update.data());

                switch (desc) {
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

                        break;
                    }
                    default: {
                        spdlog::warn("Unrecognized package with descriptor {0:x}", 
                                     std::underlying_type<tomop::packet_desc>::type(desc));
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        break;
                    }
                }
            }
        });

        preview_thread_ = std::thread([&] {
            while (true) {

                zmq::message_t reply;

                int n = recon_->previewSize();
                auto volprev = tomop::VolumeDataPacket(scene_id_, {n, n, n}, recon_->previewData());
                send(volprev);

                auto grsp = tomop::GroupRequestSlicesPacket(scene_id_, 1);
                send(grsp);
                
                spdlog::info("Volume preview data sent");
            }
        });

        thread_.detach();
        preview_thread_.detach();
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

        auto result = recon_->reconstructSlice(orientation);

        if (!result.first.empty()) {
            auto data_packet = tomop::SliceDataPacket(
                scene_id_, slice_id, result.first, std::move(result.second), false);
            send(data_packet);
        }
    }
};

} // namespace slicerecon
