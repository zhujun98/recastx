#include <string>
#include <thread>

#include <zmq.hpp>

#include "reconstructor.hpp"

namespace slicerecon {

class DataReceiver {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    std::shared_ptr<Reconstructor> recon_;

public:
    DataReceiver(const std::string& endpoint,
                 zmq::socket_type socket_type,
                 std::shared_ptr<Reconstructor> recon);

    ~DataReceiver();

    void start();
};


} // namespace slicerecon
