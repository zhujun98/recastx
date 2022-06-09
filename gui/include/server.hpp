#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include "zmq.hpp"
#include "tomop/tomop.hpp"

#include "modules/scene_module.hpp"
#include "packet_listener.hpp"
#include "ticker.hpp"

namespace gui {

class SceneList;
class SceneModuleProtocol;

class Server : public Ticker, public PacketListener {
  public:
    Server(SceneList& scenes, int port);

    void start();

    void tick(float) override;

    void handle(tomop::Packet& pkt) override;

  private:
    std::map<tomop::packet_desc, std::shared_ptr<SceneModuleProtocol>> modules_;

    SceneList& scenes_;
    std::thread thread_;

    std::queue<std::pair<tomop::packet_desc, std::unique_ptr<tomop::Packet>>> packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;

    void registerModule(std::shared_ptr<SceneModuleProtocol> module);
};

}  // namespace gui
