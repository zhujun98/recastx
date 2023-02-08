#include <spdlog/spdlog.h>

#include "recon/zmq_server.hpp"
#include "recon/application.hpp"


namespace tomcat::recon {

using namespace std::string_literals;

ZmqServer::ZmqServer(int data_port, int message_port, Application* app)
    : context_(1), 
      data_socket_(context_, ZMQ_REP),
      cmd_socket_(context_, ZMQ_PAIR),
      app_(app) {
    using namespace std::chrono_literals;

    data_socket_.bind("tcp://*:"s + std::to_string(data_port));
    cmd_socket_.bind("tcp://*:"s + std::to_string(message_port));

    spdlog::info("Waiting for connections from the GUI client. Ports: {}, {}", 
                 data_port, message_port);
}

ZmqServer::~ZmqServer() = default; 

void ZmqServer::send(const Packet& packet) {
    
    zmq::message_t msg;
    data_socket_.recv(msg, zmq::recv_flags::none);
    auto request = std::string(static_cast<char*>(msg.data()), msg.size());
    if (request == "ready") {
        packet.send(data_socket_);
    } else {
        spdlog::warn("Unknown request received: {}", request);
    }
}

void ZmqServer::start() {
    cmd_thread_ = std::thread([&] {
        while (true) {
            zmq::message_t update;
            cmd_socket_.recv(update, zmq::recv_flags::none);
            auto desc = ((PacketDesc*)update.data())[0];
            auto buffer = tomcat::memory_buffer(update.size(), (char*)update.data());

            spdlog::debug("Received packet with descriptor: 0x{0:x}", 
                          std::underlying_type<PacketDesc>::type(desc));

            switch (desc) {
                case PacketDesc::set_slice: {
                    auto packet = std::make_unique<SetSlicePacket>();
                    packet->deserialize(std::move(buffer));
                    app_->setSlice(packet->timestamp, packet->orientation);
                    break;
                }
                default: {
                    spdlog::warn("Unrecognized packet with descriptor 0x{0:x}", 
                                 std::underlying_type<PacketDesc>::type(desc));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    break;
                }
            }
        }
    });

    data_thread_ = std::thread([&] {
        while (true) {
            // - Do not block because slice request needs to be responsive
            // - If the number of the logical threads are more than the number of the physical threads, 
            //   the preview_data could always have value.
            auto preview_data = app_->previewDataPacket(0);
            if (preview_data) {
                auto slice_data = app_->sliceDataPackets(-1);

                std::lock_guard<std::mutex> lck(send_mtx_);
                send(std::move(preview_data.value()));

                spdlog::debug("Preview data sent");
                
                for (const auto& packet : slice_data) {
                    send(std::move(packet));

                    spdlog::debug("Slice data {} ({}) sent", 
                                  packet.timestamp % NUM_SLICES, packet.timestamp);

                }
            } else {
                auto slice_data = app_->onDemandSliceDataPackets(10);

                if (!slice_data.empty()) {
                    std::lock_guard<std::mutex> lck(send_mtx_);
                    for (const auto& packet : slice_data) {
                        send(std::move(packet));

                        spdlog::debug("On-demand slice data {} ({}) sent", 
                                      packet.timestamp % NUM_SLICES, packet.timestamp);

                    }
                }
            }
        }
    });

    cmd_thread_.detach();
    data_thread_.detach();
}

} // tomcat::recon