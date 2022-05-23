#pragma once

#include <cstdint>

#include "../packets.hpp"

namespace tomop {

struct ParameterBoolPacket : public PacketBase<ParameterBoolPacket> {
    static constexpr auto desc = packet_desc::parameter_bool;
    ParameterBoolPacket() = default;
    ParameterBoolPacket(int32_t a, std::string b, bool c)
        : scene_id(a), parameter_name(b), value(c) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterBoolPacket, (int32_t, scene_id),
                             (std::string, parameter_name), (bool, value));
};

struct ParameterFloatPacket : public PacketBase<ParameterFloatPacket> {
    static constexpr auto desc = packet_desc::parameter_float;
    ParameterFloatPacket() = default;
    ParameterFloatPacket(int32_t a, std::string b, float c)
        : scene_id(a), parameter_name(b), value(c) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterFloatPacket, (int32_t, scene_id),
                             (std::string, parameter_name), (float, value));
};

struct ParameterEnumPacket : public PacketBase<ParameterEnumPacket> {
    static constexpr auto desc = packet_desc::parameter_enum;
    ParameterEnumPacket() = default;
    ParameterEnumPacket(int32_t a, std::string b, std::vector<std::string> c)
        : scene_id(a), parameter_name(b), values(c) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterEnumPacket, (int32_t, scene_id),
                             (std::string, parameter_name),
                             (std::vector<std::string>, values));
};

} // namespace tomop
