#pragma once

#include <cstdint>

#include "../packets.hpp"

namespace tomop {

struct ParameterBoolPacket : public PacketBase<ParameterBoolPacket> {
    static constexpr auto desc = PacketDesc::parameter_bool;
    ParameterBoolPacket() = default;
    ParameterBoolPacket(std::string a, bool b)
        : name(a), value(b) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterBoolPacket, 
                             (std::string, name), 
                             (bool, value));
};

struct ParameterFloatPacket : public PacketBase<ParameterFloatPacket> {
    static constexpr auto desc = PacketDesc::parameter_float;
    ParameterFloatPacket() = default;
    ParameterFloatPacket(std::string b, float c)
        : name(b), value(c) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterFloatPacket, 
                             (std::string, name), 
                             (float, value));
};

struct ParameterEnumPacket : public PacketBase<ParameterEnumPacket> {
    static constexpr auto desc = PacketDesc::parameter_enum;
    ParameterEnumPacket() = default;
    ParameterEnumPacket(std::string a, std::vector<std::string> b)
        : name(a), values(b) {}
    BOOST_HANA_DEFINE_STRUCT(ParameterEnumPacket, 
                             (std::string, name),
                             (std::vector<std::string>, values));
};

} // namespace tomop
