#include <queue>
#include <memory>

#include "tomop/tomop.hpp"


namespace tomovis {
    using PacketQueue = std::queue<std::pair<tomop::packet_desc, 
                                   std::unique_ptr<tomop::Packet>>>;;
}