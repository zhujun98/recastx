#pragma once

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

#include "tomop/tomop.hpp"
#include "reconstructor.hpp"
#include "data_types.hpp"

namespace slicerecon {

class Broker {
    zmq::context_t context_;

    zmq::socket_t req_socket_;
    zmq::socket_t sub_socket_;

    std::thread thread_;
    std::thread preview_thread_;

    std::vector<std::pair<int32_t, std::array<float, 9>>> slices_;

    std::mutex socket_mutex_;

    std::shared_ptr<Reconstructor> recon_;

public:

    Broker(const std::string& endpoint,
           const std::string& subscribe_endpoint,
           std::shared_ptr<Reconstructor> recon);

    ~Broker();

    void send(const tomop::Packet& packet);

    void start();

    void makeSlice(int32_t slice_id, const std::array<float, 9>& orientation);
};

} // namespace slicerecon
