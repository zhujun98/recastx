#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "recon/daq_client.hpp"
#include "recon/application.hpp"


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


DaqClient::DaqClient(const std::string& endpoint,
                     const std::string& socket_type,
                     Application* app)
        : context_(1),
          socket_(context_, parseSocketType(socket_type)),
          app_(app) {
    socket_.connect(endpoint);

    if(socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::sub)) {
        spdlog::info("Connected to data server (PUB-SUB){}", endpoint);
        socket_.set(zmq::sockopt::subscribe, "");
    } else if (socket_.get(zmq::sockopt::type) == static_cast<int>(zmq::socket_type::pull)) {
        spdlog::info("Connected to data server (PUSH-PULL) {}", endpoint);
    }
}

DaqClient::~DaqClient() {
    socket_.set(zmq::sockopt::linger, 200);
}

void DaqClient::start() {
    thread_ = std::thread([&] {

#if (VERBOSITY >= 1)
        int monitor_every = app_->bufferSize();
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

            app_->pushProjection(proj_type,
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

zmq::socket_type DaqClient::parseSocketType(const std::string& socket_type) const {
    if (socket_type.compare("pull") == 0) return zmq::socket_type::pull;
    if (socket_type.compare("sub") == 0) return zmq::socket_type::sub;
    throw std::invalid_argument("Unsupported socket type: "s + socket_type); 
}

} // tomcat::recon