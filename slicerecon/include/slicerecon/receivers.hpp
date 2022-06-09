#include <string>
#include <thread>

#include <zmq.hpp>

#include "reconstructor.hpp"

namespace slicerecon {

class ProjectionReceiver {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    std::shared_ptr<Reconstructor> recon_;

public:
    ProjectionReceiver(const std::string& endpoint,
                      zmq::socket_type socket_type,
                      std::shared_ptr<Reconstructor> recon);

    ~ProjectionReceiver();

    void start();
};


} // namespace slicerecon
