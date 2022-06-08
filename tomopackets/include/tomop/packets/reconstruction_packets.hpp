#pragma once

#include <cstdint>

#include "../packets.hpp"

namespace tomop {

struct SliceDataPacket : public PacketBase<SliceDataPacket> {
    static constexpr auto desc = packet_desc::slice_data;
    SliceDataPacket() = default;
    SliceDataPacket(int32_t a, std::array<int32_t, 2> b,
                    std::vector<float> c, bool d)
        : slice_id(a), slice_size(b), data(c), additive(d) {}
    BOOST_HANA_DEFINE_STRUCT(SliceDataPacket, 
                             (int32_t, slice_id),
                             (std::array<int32_t, 2>, slice_size),
                             (std::vector<float>, data), 
                             (bool, additive));
};

struct VolumeDataPacket : public PacketBase<VolumeDataPacket> {
    static constexpr auto desc = packet_desc::volume_data;
    VolumeDataPacket() = default;
    VolumeDataPacket(std::array<int32_t, 3> a, std::vector<float> b)
        : volume_size(a), data(b) {}
    BOOST_HANA_DEFINE_STRUCT(VolumeDataPacket, 
                             (std::array<int32_t, 3>, volume_size),
                             (std::vector<float>, data));
};

struct SetSlicePacket : public PacketBase<SetSlicePacket> {
    static constexpr auto desc = packet_desc::set_slice;
    SetSlicePacket() = default;
    SetSlicePacket(int32_t a, std::array<float, 9> b)
        : slice_id(a), orientation(b) {}
    BOOST_HANA_DEFINE_STRUCT(SetSlicePacket, 
                             (int32_t, slice_id),
                             (std::array<float, 9>, orientation));
};

struct RemoveSlicePacket : public PacketBase<RemoveSlicePacket> {
    static constexpr auto desc = packet_desc::remove_slice;
    RemoveSlicePacket() = default;
    RemoveSlicePacket(int32_t a) 
        : slice_id(a) {}
    BOOST_HANA_DEFINE_STRUCT(RemoveSlicePacket, 
                             (int32_t, slice_id));
};

struct GroupRequestSlicesPacket : public PacketBase<GroupRequestSlicesPacket> {
    static constexpr auto desc = packet_desc::group_request_slices;
    GroupRequestSlicesPacket() = default;
    GroupRequestSlicesPacket(int32_t a)
        : group_size(a) {}
    BOOST_HANA_DEFINE_STRUCT(GroupRequestSlicesPacket, 
                             (int32_t, group_size));
};

} // namespace tomop
