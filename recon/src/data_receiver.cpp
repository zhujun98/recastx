#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "recon/data_receiver.hpp"


namespace tomcat::recon {

using namespace std::string_literals;

namespace detail {

ProjectionType parseProjectionType(int v) {
    if (v != static_cast<int>(ProjectionType::dark) &&
        v != static_cast<int>(ProjectionType::flat) && 
        v != static_cast<int>(ProjectionType::projection)) {
            return ProjectionType::unknown;
        }
    return static_cast<ProjectionType>(v);
}

} // detail


DataReceiver::DataReceiver(const std::string& endpoint,
                           zmq::socket_type socket_type,
                           std::shared_ptr<Reconstructor> recon)
        : context_(1),
          socket_(context_, socket_type),
          recon_(recon) {
    socket_.connect(endpoint);
    if(socket_type == zmq::socket_type::sub) {
        spdlog::info("Connected to data server (PUB-SUB){}", endpoint);
    } else if (socket_type == zmq::socket_type::pull) {
        spdlog::info("Connected to data server (PUSH-PULL) {}", endpoint);
    }

    if (socket_type == zmq::socket_type::sub) {
        socket_.set(zmq::sockopt::subscribe, "");
    }
}

DataReceiver::~DataReceiver() {
    socket_.close();
    context_.close();
}

void DataReceiver::start() {
    thread_ = std::thread([&] {

#if (VERBOSITY >= 1)
        int monitor_every = recon_->bufferSize();
        int msg_counter = 0;
#endif

        zmq::message_t update;
        while (true) {
            socket_.recv(update, zmq::recv_flags::none);

            auto meta = nlohmann::json::parse(std::string((char*)update.data(), update.size()));
            int frame = meta["frame"];
            int scan_index = meta["image_attributes"]["scan_index"]; 
            ProjectionType proj_type = detail::parseProjectionType(scan_index);
            if (proj_type == ProjectionType::unknown) {
                spdlog::error("Unknown scan index: {}", scan_index);
                continue;
            }
            auto shape = meta["shape"];

            socket_.recv(update, zmq::recv_flags::none);

#if (VERBOSITY >= 4)
            spdlog::info("Projection received: type = {0:d}, frame = {1:d}", scan_index, frame);
#endif

            recon_->pushProjection(proj_type,
                                   frame,
                                   {shape[0], shape[1]},
                                   static_cast<char*>(update.data()));

#if (VERBOSITY >= 1)
            if (proj_type == ProjectionType::projection) {
                ++msg_counter;
                if (msg_counter % monitor_every == 0) {
                    spdlog::info("**************************************************************");
                    spdlog::info("# of projections received: {}", msg_counter);
                    spdlog::info("**************************************************************");
                }                
            } else {
                // reset the counter and timer when dark/flat arrives
                msg_counter = 0;
            }
#endif

        }
    });

    thread_.detach();
}

} // tomcat::recon