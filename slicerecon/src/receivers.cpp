#include <chrono>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "slicerecon/data_types.hpp"
#include "slicerecon/receivers.hpp"


namespace slicerecon {

using namespace std::string_literals;

namespace detail {

// TODO: improve
ProjectionType parseProjectionType(int v) {
    if (v != static_cast<int>(ProjectionType::dark) &&
        v != static_cast<int>(ProjectionType::flat) && 
        v != static_cast<int>(ProjectionType::projection)) {
            throw std::runtime_error("Unsupported scan_index value: "s + std::to_string(v));
        }
    return static_cast<ProjectionType>(v);
}

} // detail


ProjectionReceiver::ProjectionReceiver(const std::string& endpoint,
                                       zmq::socket_type socket_type,
                                       std::shared_ptr<Reconstructor> recon)
        : context_(1),
          socket_(context_, socket_type),
          recon_(recon) {
    if(socket_type == zmq::socket_type::sub) {
        spdlog::info("Subscribing to data source {} ...", endpoint);
    } else if (socket_type == zmq::socket_type::pull) {
        spdlog::info("Pulling data from data source {} ...", endpoint);
    }
    socket_.connect(endpoint);

    if (socket_type == zmq::socket_type::sub) {
        socket_.set(zmq::sockopt::subscribe, "");
    }
}

ProjectionReceiver::~ProjectionReceiver() {
    socket_.close();
    context_.close();
}

void ProjectionReceiver::start() {
    thread_ = std::thread([&] {
#if defined(WITH_MONITOR)
        int monitor_every = recon_->bufferSize();
        int msg_counter = 0;
        float msg_size = 0.f;  // in MB
        auto start = std::chrono::steady_clock::now();
#endif
        zmq::message_t update;
        while (true) {
            socket_.recv(update, zmq::recv_flags::none);

            auto meta = nlohmann::json::parse(std::string((char*)update.data(), update.size()));
            int frame = meta["frame"];
            ProjectionType scan_index = detail::parseProjectionType(
                meta["image_attributes"]["scan_index"]);
            auto shape = meta["shape"];

            socket_.recv(update, zmq::recv_flags::none);
            if (scan_index == ProjectionType::dark || scan_index == ProjectionType::flat) {
                spdlog::info("Projection received: type = {0:d}, frame = {1:d}", 
                                static_cast<int>(scan_index), frame);
            }

            recon_->pushProjection(scan_index,
                                    frame,
                                    {shape[0], shape[1]},
                                    static_cast<char*>(update.data()));

#if defined(WITH_MONITOR)
            if (!msg_size) {
                msg_size = static_cast<float>(shape[0]) * static_cast<float>(shape[1])
                            * sizeof(RawDtype) / (1024 * 1024);
            }
            if (scan_index == ProjectionType::projection) {
                ++msg_counter;
                if (msg_counter % monitor_every == 0) {
                    float duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() -  start).count();
                    spdlog::info("**************************************************************");
                    spdlog::info("# of projections received: {}, throughput (MB/s): {}", 
                                    msg_counter, msg_size * monitor_every * 1000000 / (duration));
                    spdlog::info("**************************************************************");
                    start = std::chrono::steady_clock::now();
                }                
            } else {
                // reset the counter and timer when dark/flat arrives
                msg_counter = 0;
                start = std::chrono::steady_clock::now();
            }

#endif
        }
    });

    thread_.detach();
}


} // namespace slicerecon