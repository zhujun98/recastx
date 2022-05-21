#pragma once

#include <iostream>

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "scene.hpp"
#include "scene_module.hpp"

#include "graphics/components/partitioning_component.hpp"

namespace gui {

class PartitioningProtocol : public SceneModuleProtocol {
  public:
    std::unique_ptr<tomop::Packet> read_packet(tomop::packet_desc desc,
                                               tomop::memory_buffer& buffer,
                                               zmq::socket_t& socket,
                                               SceneList& /* scenes_ */) override {
        switch (desc) {
            case tomop::packet_desc::set_part: {
                auto packet = std::make_unique<tomop::SetPartPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            default: { return nullptr; }
            }
        }

    void process(SceneList& scenes,
                 tomop::packet_desc desc,
                 std::unique_ptr<tomop::Packet> event_packet) override {
        switch (desc) {
            case tomop::packet_desc::set_part: {
                tomop::SetPartPacket& packet = *(tomop::SetPartPacket*)event_packet.get();

                auto scene = scenes.get_scene(packet.scene_id);
                if (!scene) {
                    std::cout << "Updating non-existing scene\n";
                    return;
                }
                auto& part_component =
                    (PartitioningComponent&)scene->object().get_component("partitioning");
                auto min_pt = packet.min_pt;
                auto max_pt = packet.max_pt;
                part_component.add_part(part(packet.part_id,
                                            {min_pt[0], min_pt[1], min_pt[2]},
                                            {max_pt[0], max_pt[1], max_pt[2]}));
                break;
            }
            default: { break; }
        }
    }

    std::vector<tomop::packet_desc> descriptors() override {
        return {tomop::packet_desc::set_part};
    }
};

} // namespace gui
