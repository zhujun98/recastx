#include <queue>
#include <memory>

#include "tomop/tomop.hpp"


namespace tomovis {
    using PacketQueue = std::queue<std::pair<tomop::PacketDesc, 
                                   std::unique_ptr<tomop::Packet>>>;;
}