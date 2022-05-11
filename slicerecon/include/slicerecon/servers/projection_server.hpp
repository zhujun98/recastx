#include <string>
#include <thread>
#include <chrono>

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"

#include "../util/exceptions.hpp"
#include "../data_types.hpp"

namespace slicerecon {

using namespace std::string_literals;

namespace detail {
    // TODO: improve
    ProjectionType parseProjectionType(int v) {
        if (v != static_cast<int>(ProjectionType::dark) &&
            v != static_cast<int>(ProjectionType::flat) && 
            v != static_cast<int>(ProjectionType::standard)) {
                throw std::runtime_error("Unsupported scan_index value: "s + std::to_string(v));
            }
        return static_cast<ProjectionType>(v);
    }
} // detail

class ProjectionServer {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread serve_thread_;

public:
    ProjectionServer(const std::string& hostname,
                     int port, 
                     zmq::socket_type socket_type)
        : context_(1),
          socket_(context_, socket_type) {
        auto address = "tcp://"s + hostname + ":"s + std::to_string(port);
        if(socket_type == zmq::socket_type::sub) {
            spdlog::info("Subscribing to data source {} ...", address);
        } else if (socket_type == zmq::socket_type::pull) {
            spdlog::info("Pulling data from data source {} ...", address);
        }
        socket_.connect(address);

        if (socket_type == zmq::socket_type::sub) {
            socket_.set(zmq::sockopt::subscribe, "");
        }
    }

    ~ProjectionServer() {
        if (serve_thread_.joinable()) serve_thread_.join();
        socket_.close();
        context_.close();
    }

    void start(Reconstructor& recon, const Settings& params) {
        serve_thread_ = std::thread([&] {
#if defined(WITH_MONITOR)
            int monitor_every = params.recon_mode == slicerecon::Mode::alternating 
                                ? params.projections : params.group_size;
            int msg_counter = 0;
            int msg_size = 0;  // in MB
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

                recon.pushProjection(scan_index,
                                     frame, 
                                     {shape[0], shape[1]}, 
                                     static_cast<char*>(update.data()));

#if defined(WITH_MONITOR)
                if (!msg_size) {
                    msg_size = static_cast<int>(shape[0]) * static_cast<int>(shape[1])
                               * sizeof(raw_dtype) / (1024 * 1024);
                }
                if (scan_index == ProjectionType::standard) {
                    ++msg_counter;
                    if (msg_counter % monitor_every == 0) {
                        float duration = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() -  start).count();
                        spdlog::info("# of projections received: {}", msg_counter);
                        spdlog::info("Throughput (MB/s): {}", 
                                     msg_size * monitor_every * 1000000 / (duration));
                        start = std::chrono::steady_clock::now();
                    }                
                } else {
                    // reset the timer when dark/flat arrives
                    start = std::chrono::steady_clock::now();
                }

#endif
            }
        });
    }
};


} // namespace slicerecon
