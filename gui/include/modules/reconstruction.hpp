#pragma once

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "scene.hpp"
#include "scene_module.hpp"

#include "graphics/components/reconstruction_component.hpp"
#include "util.hpp"

namespace gui {

// for the 'one-way-communication' we have two parts
// a handler that knows how to read in a packet
// and an executor that knows how to execute a packet

class ReconstructionProtocol : public SceneModuleProtocol {
  public:
    std::unique_ptr<tomop::Packet>
    read_packet(tomop::packet_desc desc,
                tomop::memory_buffer& buffer,
                zmq::socket_t& socket,
                SceneList& /* scenes_ */) override {
        switch (desc) {
            case tomop::packet_desc::slice_data: {
                auto packet = std::make_unique<tomop::SliceDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }

            case tomop::packet_desc::partial_slice_data: {
                auto packet = std::make_unique<tomop::PartialSliceDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }

            case tomop::packet_desc::volume_data: {
                auto packet = std::make_unique<tomop::VolumeDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }

            case tomop::packet_desc::partial_volume_data: {
                auto packet = std::make_unique<tomop::PartialVolumeDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }

            case tomop::packet_desc::group_request_slices: {
                auto packet = std::make_unique<tomop::GroupRequestSlicesPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }

            default: { return nullptr; }
        }
    }

    void process(SceneList& scenes, tomop::packet_desc desc,
                 std::unique_ptr<tomop::Packet> event_packet) override {
        switch (desc) {
            case tomop::packet_desc::slice_data: {
                tomop::SliceDataPacket& packet = *(tomop::SliceDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";

                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component("reconstruction");
                reconstruction_component.set_data(packet.data, 
                                                  packet.slice_size,
                                                  packet.slice_id, 
                                                  packet.additive);
                break;
            }
            case tomop::packet_desc::partial_slice_data: {
                tomop::PartialSliceDataPacket& packet =
                    *(tomop::PartialSliceDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";

                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component("reconstruction");
                reconstruction_component.update_partial_slice(
                    packet.data, packet.slice_offset, packet.slice_size,
                    packet.global_slice_size, packet.slice_id, packet.additive);
                break;
            }

            case tomop::packet_desc::volume_data: {
                tomop::VolumeDataPacket& packet = *(tomop::VolumeDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";

                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component("reconstruction");
                reconstruction_component.set_volume_data(packet.data, packet.volume_size);
                break;
            }
            case tomop::packet_desc::partial_volume_data: {
                tomop::PartialVolumeDataPacket& packet =
                    *(tomop::PartialVolumeDataPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) std::cout << "Updating non-existing scene\n";

                auto& reconstruction_component =
                    (ReconstructionComponent&)scene->object().get_component("reconstruction");
                reconstruction_component.update_partial_volume(
                    packet.data, packet.volume_offset, packet.volume_size,
                    packet.global_volume_size);
                break;
            }
            case tomop::packet_desc::group_request_slices: {
                tomop::GroupRequestSlicesPacket& packet = *(tomop::GroupRequestSlicesPacket*)event_packet.get();
                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) {
                    std::cout << "Updating non-existing scene\n";
                }

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
                        (ReconstructionComponent&)scene->object().get_component("reconstruction");
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

    std::vector<tomop::packet_desc> descriptors() override {
        return {tomop::packet_desc::slice_data,
                tomop::packet_desc::partial_slice_data,
                tomop::packet_desc::volume_data,
                tomop::packet_desc::partial_volume_data,
                tomop::packet_desc::group_request_slices};
    }

  private:
    int group_size_count_ = -1;
    int group_size_requested_ = -1;
};

} // namespace gui
