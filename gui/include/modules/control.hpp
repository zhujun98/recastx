#include <vector>

#include "tomop/tomop.hpp"
#include "zmq.hpp"

#include "scene.hpp"
#include "scene_module.hpp"

#include "util.hpp"

#include "graphics/control_component.hpp"

namespace gui {

// Module for analysis and control
class ControlProtocol : public SceneModuleProtocol {
  public:
    std::unique_ptr<tomop::Packet> read_packet(tomop::packet_desc desc,
                                               tomop::memory_buffer& buffer,
                                               zmq::socket_t& socket,
                                               SceneList& /* scenes */) override {
        switch (desc) {
            case tomop::packet_desc::parameter_bool: {
                auto packet = std::make_unique<tomop::ParameterBoolPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::packet_desc::parameter_float: {
                auto packet = std::make_unique<tomop::ParameterFloatPacket>();
                packet->deserialize(std::move(buffer));
                ack(socket);
                return packet;
            }
            case tomop::packet_desc::parameter_enum: {
                auto packet = std::make_unique<tomop::ParameterEnumPacket>();
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

        auto get_component = [&](int scene_id) -> ControlComponent* {
            auto scene = scenes.get_scene(scene_id);
            if (!scene) {
                std::cout << "Updating non-existing scene\n";
                return nullptr;
            }
            auto control_component =
                &(ControlComponent&)scene->object().get_component("control");

            return control_component;
        };

        switch (desc) {
        case tomop::packet_desc::parameter_bool: {
            auto& packet = *(tomop::ParameterBoolPacket*)event_packet.get();
            auto control_component = get_component(packet.scene_id);
            if (!control_component) return;
            control_component->add_bool_parameter(packet.parameter_name, packet.value);
            break;
        }
        case tomop::packet_desc::parameter_float: {
            auto& packet = *(tomop::ParameterFloatPacket*)event_packet.get();
            auto control_component = get_component(packet.scene_id);
            if (!control_component) return;
            control_component->add_float_parameter(packet.parameter_name, packet.value);
            break;
        }
        case tomop::packet_desc::parameter_enum: {
            auto& packet = *(tomop::ParameterEnumPacket*)event_packet.get();
            auto control_component = get_component(packet.scene_id);
            if (!control_component) return;
            control_component->add_enum_parameter(packet.parameter_name, packet.values);
            break;
        }
        default: { break; }
        }
    }

    std::vector<tomop::packet_desc> descriptors() override {
        return {tomop::packet_desc::parameter_bool,
                tomop::packet_desc::parameter_float,
                tomop::packet_desc::parameter_enum};
    }
};

} // namespace gui
