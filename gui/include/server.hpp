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

namespace tomop::gui {

class SceneModuleProtocol;

class Server : public Ticker, public PacketPublisher {

    std::map<PacketDesc, std::shared_ptr<SceneModuleProtocol>> modules_;

    MainWindow& window_;
    std::thread thread_;

    std::queue<std::pair<PacketDesc, std::unique_ptr<Packet>>> packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void sendPacket(Packet& packet) override;

    void registerModule(const std::shared_ptr<SceneModuleProtocol>& module);
  
  public:

    Server(MainWindow& window, int port);

    void start();

    void tick(float) override;
};

}  // tomop::gui
