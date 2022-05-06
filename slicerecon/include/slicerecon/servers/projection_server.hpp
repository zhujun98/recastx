#include <string>
#include <thread>

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "tomop/tomop.hpp"

#include "../util/exceptions.hpp"
#include "../data_types.hpp"

namespace slicerecon {

class ProjectionServer {

    zmq::context_t context_;
    zmq::socket_t socket_;

    Reconstructor& recon_;

    std::thread serve_thread_;

public:
    ProjectionServer(const std::string& hostname,
                     int port, 
                     Reconstructor& recon, 
                     zmq::socket_type socket_type)
        : context_(1),
          socket_(context_, socket_type),
          recon_(recon) {
        using namespace std::string_literals;
        auto address = "tcp://"s + hostname + ":"s + std::to_string(port);
        spdlog::info("Connecting to data source {} ...", address);
        socket_.connect(address);
    }

    ~ProjectionServer() {
        if (serve_thread_.joinable()) serve_thread_.join();
        socket_.close();
        context_.close();
    }

    void serve() {
        serve_thread_ = std::thread([&] {
            zmq::message_t update;
            while (true) {
                socket_.recv(update, zmq::recv_flags::none);

                auto meta = nlohmann::json::parse(std::string((char*)update.data(), update.size()));
                int frame = meta["frame"];
                int scan_idx = meta["image_attributes"]["scan_index"];
                if (scan_idx < 0 || scan_idx > 2) {
                    spdlog::error("scan_index must be either 0, 1 or 2. Actual: {}", scan_idx);
                    continue;
                }
                auto shape = meta["shape"];

                spdlog::info("Projection received: type = {0:d}, frame = {1:d}", scan_idx, frame);
                socket_.recv(update, zmq::recv_flags::none);

                recon_.pushProjection(static_cast<ProjectionType>(scan_idx), 
                                      frame, 
                                      {shape[0], shape[1]}, 
                                      static_cast<char*>(update.data()));
            }
        });
    }
};

} // namespace slicerecon
