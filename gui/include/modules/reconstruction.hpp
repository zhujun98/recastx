#pragma once

#include <zmq.hpp>
#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"
#include "graphics/recon_component.hpp"
#include "window.hpp"
#include "scene_module.hpp"
#include "util.hpp"

namespace gui {

class ReconstructionProtocol : public SceneModuleProtocol {

  public:

    ReconstructionProtocol() = default;
    ~ReconstructionProtocol() override = default;

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
            default: { return nullptr; }
        }
    }

    void process(MainWindow& window,
                 tomop::PacketDesc desc,
                 std::unique_ptr<tomop::Packet> event_packet) override {
        switch (desc) {
            case tomop::PacketDesc::slice_data: {
                tomop::SliceDataPacket& packet = *(tomop::SliceDataPacket*)event_packet.get();

                auto& recon_component = (ReconComponent&)window.scene().get_component("reconstruction");
                recon_component.setSliceData(packet.data, packet.slice_size, packet.slice_id);
                spdlog::info("Set slice data {}", packet.slice_id);
                break;
            }
            case tomop::PacketDesc::volume_data: {
                tomop::VolumeDataPacket& packet = *(tomop::VolumeDataPacket*)event_packet.get();

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

    std::vector<tomop::PacketDesc> descriptors() override {
        return {tomop::PacketDesc::slice_data,
                tomop::PacketDesc::volume_data,
                tomop::PacketDesc::group_request_slices};
    }

};

} // namespace gui
