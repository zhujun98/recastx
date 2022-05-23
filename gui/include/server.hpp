// TODO: we want some 'server system' that supports a number of operations:
// - Adding a scene,
// - Updating images / slices.
// - Benchmark data?
//
// I guess we want to use OpenMQ for this. Do we want two-way communication or
// just 'fire'? The nice thing about 'two-way' is that this 'slice
// reconstruction' thing can also be done. At first simply only 'sending' the
// appropriate slice, but later restricting construction only to the request.

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include "zmq.hpp"
#include "tomop/tomop.hpp"

#include "packet_listener.hpp"
#include "ticker.hpp"

namespace gui {

class SceneList;
class SceneModuleProtocol;

class Server : public Ticker, public PacketListener {
  public:
    Server(SceneList& scenes, int port);

    void start();

    void register_module(std::shared_ptr<SceneModuleProtocol> module);

    void tick(float) override;

    void handle(tomop::Packet& pkt) override;

  private:
    std::map<tomop::packet_desc, std::shared_ptr<SceneModuleProtocol>> modules_;

    SceneList& scenes_;
    std::thread server_thread_;

    std::queue<std::pair<tomop::packet_desc, std::unique_ptr<tomop::Packet>>> packets_;

    zmq::context_t context_;
    zmq::socket_t rep_socket_;
    zmq::socket_t pub_socket_;
};

}  // namespace gui
