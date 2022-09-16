#ifndef TOMCAT_H
#define TOMCAT_H

#include "packets.hpp"
#include "packets/reconstruction_packets.hpp"
#include "packets/control_packets.hpp"

namespace tomcat {

    using PacketDataEvent = std::pair<PacketDesc, std::unique_ptr<Packet>>;

    using Orientation = std::array<float, 9>;
    using SliceData = std::pair<std::array<int32_t, 2>, std::vector<float>>;

    enum class ProjectionType : int32_t { dark = 0, flat = 1, projection = 2, unknown = 99 };

    using RawDtype = uint16_t;

    using RawImageData = std::vector<RawDtype>;
    using ImageData = std::vector<float>;

} // tomcat

#endif // TOMCAT_H