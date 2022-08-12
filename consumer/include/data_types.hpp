#include <queue>
#include <memory>

#include "tomcat/tomcat.hpp"


namespace tomcat::consumer {
    using PacketQueue = std::queue<std::pair<PacketDesc, std::unique_ptr<Packet>>>;;
}