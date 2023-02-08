#pragma once

#include <cstdint>

#include "../packets.hpp"

namespace tomcat {

struct SliceDataPacket : public PacketBase<SliceDataPacket> {
    static constexpr auto desc = PacketDesc::slice_data;
    SliceDataPacket() = default;
    SliceDataPacket(int32_t a, std::array<uint32_t, 2> b,
                    std::vector<float> c)
        : timestamp(a), shape(b), data(c) {}
    BOOST_HANA_DEFINE_STRUCT(SliceDataPacket, 
                             (int32_t, timestamp),
                             (std::array<uint32_t, 2>, shape),
                             (std::vector<float>, data));
};

struct VolumeDataPacket : public PacketBase<VolumeDataPacket> {
    static constexpr auto desc = PacketDesc::volume_data;
    VolumeDataPacket() = default;
    VolumeDataPacket(std::array<uint32_t, 3> a, std::vector<float> b)
        : shape(a), data(b) {}
    BOOST_HANA_DEFINE_STRUCT(VolumeDataPacket, 
                             (std::array<uint32_t, 3>, shape),
                             (std::vector<float>, data));
};

struct SetSlicePacket : public PacketBase<SetSlicePacket> {
    static constexpr auto desc = PacketDesc::set_slice;
    SetSlicePacket() = default;
    SetSlicePacket(int32_t a, std::array<float, 9> b)
        : timestamp(a), orientation(b) {}
    BOOST_HANA_DEFINE_STRUCT(SetSlicePacket, 
                             (int32_t, timestamp),
                             (std::array<float, 9>, orientation));
};

} // namespace tomcat
