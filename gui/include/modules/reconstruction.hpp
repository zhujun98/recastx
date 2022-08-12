#pragma once

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "tomcat/tomcat.hpp"
#include "graphics/recon_component.hpp"
#include "window.hpp"
#include "scene_module.hpp"
#include "util.hpp"

namespace tomcat::gui {

class ReconstructionProtocol : public SceneModuleProtocol {

  public:

    ReconstructionProtocol() = default;
    ~ReconstructionProtocol() override = default;

    std::unique_ptr<Packet> readPacket(
            PacketDesc desc,
            tomcat::memory_buffer& buffer,
            zmq::socket_t& socket) override {
        switch (desc) {
            case PacketDesc::slice_data: {
                auto packet = std::make_unique<SliceDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case PacketDesc::volume_data: {
                auto packet = std::make_unique<VolumeDataPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            default: { return nullptr; }
        }
    }

    void process(MainWindow& window,
                 PacketDesc desc,
                 std::unique_ptr<Packet> event_packet) override {
        switch (desc) {
            case PacketDesc::slice_data: {
                SliceDataPacket& packet = *(SliceDataPacket*)event_packet.get();

                auto& recon_component = (ReconComponent&)window.scene().get_component("reconstruction");
                recon_component.setSliceData(packet.data, packet.slice_size, packet.slice_id);
                spdlog::info("Set slice data {}", packet.slice_id);
                break;
            }
            case PacketDesc::volume_data: {
                VolumeDataPacket& packet = *(VolumeDataPacket*)event_packet.get();

                auto& recon_component = (ReconComponent&)window.scene().get_component("reconstruction");
                recon_component.setVolumeData(packet.data, packet.volume_size);
                spdlog::info("Set volume data");
                break;
            }
            default: {
                std::cout << "Reconstruction module ignoring an unknown packet..\n";
                break;
            }
        }
    }

    std::vector<PacketDesc> descriptors() override {
        return {PacketDesc::slice_data,
                PacketDesc::volume_data};
    }

};

} // tomcat::gui
