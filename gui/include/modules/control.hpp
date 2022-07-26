#include <vector>

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "scene.hpp"
#include "scene_module.hpp"

#include "util.hpp"

#include "graphics/control_component.hpp"

namespace gui {

class ControlProtocol : public SceneModuleProtocol {
  public:
    std::unique_ptr<tomop::Packet> readPacket(
            tomop::PacketDesc desc,
            tomop::memory_buffer& buffer,
            zmq::socket_t& socket) override {
        switch (desc) {
            case tomop::PacketDesc::parameter_bool: {
                auto packet = std::make_unique<tomop::ParameterBoolPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::PacketDesc::parameter_float: {
                auto packet = std::make_unique<tomop::ParameterFloatPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::PacketDesc::parameter_enum: {
                auto packet = std::make_unique<tomop::ParameterEnumPacket>();
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
            case tomop::PacketDesc::parameter_bool: {
                auto& packet = *(tomop::ParameterBoolPacket*)event_packet.get();
                auto& control_component = (ControlComponent&)scenes.object().get_component("control");
                control_component.add_bool_parameter(packet.name, packet.value);
                break;
            }
            case tomop::PacketDesc::parameter_float: {
                auto& packet = *(tomop::ParameterFloatPacket*)event_packet.get();
                auto& control_component = (ControlComponent&)scenes.object().get_component("control");
                control_component.add_float_parameter(packet.name, packet.value);
                break;
            }
            case tomop::PacketDesc::parameter_enum: {
                auto& packet = *(tomop::ParameterEnumPacket*)event_packet.get();
                auto& control_component = (ControlComponent&)scenes.object().get_component("control");
                control_component.add_enum_parameter(packet.name, packet.values);
                break;
            }
            default: { break; }
        }
    }

    std::vector<tomop::PacketDesc> descriptors() override {
        return {tomop::PacketDesc::parameter_bool,
                tomop::PacketDesc::parameter_float,
                tomop::PacketDesc::parameter_enum};
    }
};

} // namespace gui
