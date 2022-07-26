#pragma once

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"
#include "graphics/reconstruction_component.hpp"
#include "scene.hpp"
#include "scene_module.hpp"
#include "util.hpp"

namespace gui {

class ReconstructionProtocol : public SceneModuleProtocol {

    int group_size_count_ = -1;
    int group_size_requested_ = -1;

  public:

    std::unique_ptr<tomop::Packet> readPacket(
            tomop::PacketDesc desc,
            tomop::memory_buffer& buffer,
            zmq::socket_t& socket) override {
        switch (desc) {
            case tomop::PacketDesc::slice_data: {
                auto packet = std::make_unique<tomop::SliceDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::PacketDesc::volume_data: {
                auto packet = std::make_unique<tomop::VolumeDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::PacketDesc::group_request_slices: {
                auto packet = std::make_unique<tomop::GroupRequestSlicesPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            default: { return nullptr; }
        }
    }

    void process(SceneList& scenes, 
                 tomop::PacketDesc desc,
                 std::unique_ptr<tomop::Packet> event_packet) override {
        switch (desc) {
            case tomop::PacketDesc::slice_data: {
                tomop::SliceDataPacket& packet = *(tomop::SliceDataPacket*)event_packet.get();

                auto& reconstruction_component =
                    (ReconstructionComponent&)scenes.object().get_component("reconstruction");
                reconstruction_component.set_data(packet.data, 
                                                  packet.slice_size,
                                                  packet.slice_id, 
                                                  packet.additive);
                spdlog::info("Set slice data {}", packet.slice_id);
                break;
            }
            case tomop::PacketDesc::volume_data: {
                tomop::VolumeDataPacket& packet = *(tomop::VolumeDataPacket*)event_packet.get();

                auto& reconstruction_component =
                    (ReconstructionComponent&)scenes.object().get_component("reconstruction");
                reconstruction_component.set_volume_data(packet.data, packet.volume_size);
                spdlog::info("Set volume data");
                break;
            }

            case tomop::PacketDesc::group_request_slices: {
                tomop::GroupRequestSlicesPacket& packet = *(tomop::GroupRequestSlicesPacket*)event_packet.get();

                if (group_size_requested_ < 0) {
                    group_size_requested_ = packet.group_size;
                    group_size_count_ = 1;
                } else {
                    if (group_size_requested_ != packet.group_size) {
                        std::cout << "Group request for different group sizes "
                                  << group_size_requested_
                                  << " != " << packet.group_size << "\n";
                    }
                    group_size_count_ += 1;
                }

                if (group_size_count_ == group_size_requested_) {
                    group_size_count_ = -1;
                    group_size_requested_ = -1;

                    auto& reconstruction_component =
                        (ReconstructionComponent&)scenes.object().get_component("reconstruction");
                    reconstruction_component.send_slices();
                }

                break;
            }
            default: {
                std::cout << "Reconstruction module ignoring an unknown packet..\n";
                break;
            }
        }
    }

    std::vector<tomop::PacketDesc> descriptors() override {
        return {tomop::PacketDesc::slice_data,
                tomop::PacketDesc::volume_data,
                tomop::PacketDesc::group_request_slices};
    }

};

} // namespace gui
