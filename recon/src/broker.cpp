#include <spdlog/spdlog.h>

#include "recon/broker.hpp"


namespace tomcat::recon {

using namespace std::string_literals;

Broker::Broker(int port, std::shared_ptr<Reconstructor> recon)
    : context_(1), 
      data_socket_(context_, ZMQ_REP),
      cmd_socket_(context_, ZMQ_PAIR),
      recon_(recon) {
    using namespace std::chrono_literals;

    data_socket_.bind("tcp://*:"s + std::to_string(port));
    cmd_socket_.bind("tcp://*:"s + std::to_string(port + 1));

    spdlog::info("Waiting for connections from the GUI client. Ports: {}, {}", 
                 port, port + 1);
}

Broker::~Broker() = default; 

void Broker::send(const Packet& packet) {
    
    zmq::message_t msg;
    data_socket_.recv(msg, zmq::recv_flags::none);
    auto request = std::string(static_cast<char*>(msg.data()), msg.size());
    if (request == "Hello recon") {
        packet.send(data_socket_);
    } else {
        spdlog::warn("Unknown request received: {}", request);
    }
}

void Broker::start() {
    cmd_thread_ = std::thread([&] {
        while (true) {
            zmq::message_t update;
            cmd_socket_.recv(update, zmq::recv_flags::none);
            auto desc = ((PacketDesc*)update.data())[0];
            auto buffer = tomcat::memory_buffer(update.size(), (char*)update.data());

#if (VERBOSITY >= 3)
            spdlog::info("Received packet with descriptor: 0x{0:x}", 
                         std::underlying_type<PacketDesc>::type(desc));
#endif

            switch (desc) {
                case PacketDesc::set_slice: {
                    auto packet = std::make_unique<SetSlicePacket>();
                    packet->deserialize(std::move(buffer));
                    recon_->setSlice(packet->slice_id, packet->orientation);
                    break;
                }
                case PacketDesc::remove_slice: {
                    auto packet = std::make_unique<RemoveSlicePacket>();
                    packet->deserialize(std::move(buffer));
                    recon_->removeSlice(packet->slice_id);
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

    data_thread1_ = std::thread([&] {
        while (true) {
            auto preview_data = recon_->previewDataPacket();
            auto slice_data = recon_->sliceDataPackets();

            std::lock_guard<std::mutex> lck(send_mtx_);

            send(std::move(preview_data));
            spdlog::info("Volume preview data sent");
            
            for (const auto& packet : slice_data) {
                send(std::move(packet));
                spdlog::info("Slice data {} sent", packet.slice_id);
            }
        }
    });

    data_thread2_ = std::thread([&] {
        while (true) {
            auto slice_data = recon_->updatedSliceDataPackets();

            std::lock_guard<std::mutex> lck(send_mtx_);
            for (const auto& packet : slice_data) {
                send(std::move(packet));
                spdlog::info("Slice data {} sent", packet.slice_id);
            }
        }
    });

    cmd_thread_.detach();
    data_thread1_.detach();
    data_thread2_.detach();
}

} // tomcat::recon