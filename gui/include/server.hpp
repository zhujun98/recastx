#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include "zmq.hpp"
#include "tomop/tomop.hpp"

#include "modules/scene_module.hpp"
#include "packet_publisher.hpp"
#include "ticker.hpp"

namespace gui {

class SceneList;
class SceneModuleProtocol;

class Server : public Ticker, public PacketPublisher {

    std::map<tomop::PacketDesc, std::shared_ptr<SceneModuleProtocol>> modules_;

    SceneList& scenes_;
    std::thread thread_;

    std::queue<std::pair<tomop::PacketDesc, std::unique_ptr<tomop::Packet>>> packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void sendPacket(tomop::Packet& packet) override;

    void registerModule(const std::shared_ptr<SceneModuleProtocol>& module);
  
  public:

    Server(SceneList& scenes, int port);

    void start();

    void tick(float) override;
};

}  // namespace gui
