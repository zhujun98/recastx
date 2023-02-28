#include <spdlog/spdlog.h>

#include "recon/zmq_server.hpp"
#include "recon/application.hpp"

#include "common/utils.hpp"

namespace tomcat::recon {

using namespace std::string_literals;

DataServer::DataServer(int port, Application* app)
    : context_(1), socket_(context_, ZMQ_REP), app_(app) {

    socket_.bind("tcp://*:"s + std::to_string(port));

    spdlog::info("Waiting for connections from the data client: {}", port);
}

DataServer::~DataServer() = default;

void DataServer::start() {
    thread_ = std::thread([&] {
        while (true) {
            // - Do not block because slice request needs to be responsive
            // - If the number of the logical threads are more than the number of the physical threads, 
            //   the preview_data could always have value.
            auto preview_data = app_->previewDataPacket(0);
            if (preview_data) {
                auto slice_data = app_->sliceDataPackets(-1);

                std::lock_guard<std::mutex> lck(send_mtx_);

                send(preview_data.value());
                
                for (const auto& packet : slice_data) {
                    send(packet);
                    auto ts = packet.slice().timestamp();
                    spdlog::debug("Slice data {} ({}) sent", sliceIdFromTimestamp(ts), ts);
                }
            } else {
                auto slice_data = app_->onDemandSliceDataPackets(10);

                if (!slice_data.empty()) {
                    std::lock_guard<std::mutex> lck(send_mtx_);
                    for (const auto& packet : slice_data) {
                        send(packet);
                        auto ts = packet.slice().timestamp();
                        spdlog::debug("On-demand slice data {} ({}) sent", 
                                      sliceIdFromTimestamp(ts), ts);
                    }
                }
            }
        }
    });

    thread_.detach();
}


MessageServer::MessageServer(int port, Application* app)
    : context_(1), socket_(context_, ZMQ_PAIR), app_(app) {
    using namespace std::chrono_literals;

    socket_.bind("tcp://*:"s + std::to_string(port));

    spdlog::info("Waiting for the connection from the message client: {}", port);
}

MessageServer::~MessageServer() = default; 

void MessageServer::start() {
    thread_ = std::thread([&] {
        while (true) {
            zmq::message_t update;
            socket_.recv(update, zmq::recv_flags::none);

            ReconRequestPacket packet;
            packet.ParseFromArray(update.data(), static_cast<int>(update.size()));

            spdlog::debug("Received packet");

            if (packet.has_set_slice()) {
                auto& request = packet.set_slice();
                Orientation orient;
                std::copy(request.orientation().begin(), request.orientation().end(), orient.begin());
                app_->setSlice(request.timestamp(), orient);
            } else {
                spdlog::warn("Unknown or empty packet received");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
        }
    });

    thread_.detach();
}

} // tomcat::recon