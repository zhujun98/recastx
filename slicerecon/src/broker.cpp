#include <spdlog/spdlog.h>

#include "slicerecon/broker.hpp"


namespace tomop::slicerecon {

Broker::Broker(const std::string& endpoint,
               const std::string& subscribe_endpoint,
               std::shared_ptr<Reconstructor> recon)
    : context_(1), 
      req_socket_(context_, ZMQ_REQ),
      sub_socket_(context_, ZMQ_SUB),
      recon_(recon) {
    using namespace std::chrono_literals;

    req_socket_.set(zmq::sockopt::linger, 200);
    req_socket_.connect(endpoint);
    spdlog::info("Connected to GUI server (REQ-REP): {}", endpoint);

    sub_socket_.connect(subscribe_endpoint);
    spdlog::info("Connected to GUI server (PUB-SUB): {}", subscribe_endpoint);

    std::vector<PacketDesc> descriptors = {
        PacketDesc::set_slice,
        PacketDesc::remove_slice
    };

    for (auto descriptor : descriptors) {
        int32_t filter[] = {
            (std::underlying_type<PacketDesc>::type)descriptor, 0
        };
        sub_socket_.set(zmq::sockopt::subscribe, zmq::buffer(filter));
    }
}

Broker::~Broker() {
    req_socket_.close();
    sub_socket_.close();
    context_.close();
}

void Broker::send(const Packet& packet) {
    zmq::message_t reply;
    packet.send(req_socket_);
    req_socket_.recv(reply, zmq::recv_flags::none);
}

void Broker::start() {
    sub_thread_ = std::thread([&] {
        while (true) {
            zmq::message_t update;
            sub_socket_.recv(update, zmq::recv_flags::none);
            auto desc = ((PacketDesc*)update.data())[0];
            auto buffer = tomop::memory_buffer(update.size(), (char*)update.data());

#if (VERBOSITY >= 3)
            spdlog::info("Received packet with descriptor: 0x{0:x}", 
                         std::underlying_type<PacketDesc>::type(desc));
#endif

            switch (desc) {
                case PacketDesc::set_slice: {
                    auto packet = std::make_unique<SetSlicePacket>();
                    packet->deserialize(std::move(buffer));
                    recon_->setSlice(packet->slice_id, packet->orientation);

#if (VERBOSITY >= 3)
                    spdlog::info("Set slice {}", packet->slice_id);
#endif

                    break;
                }
                case PacketDesc::remove_slice: {
                    auto packet = std::make_unique<RemoveSlicePacket>();
                    packet->deserialize(std::move(buffer));
                    recon_->removeSlice(packet->slice_id);

#if (VERBOSITY >= 3)
                    spdlog::info("Remove slice {}", packet->slice_id);
#endif

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

    req_thread_ = std::thread([&] {
        while (true) {
            zmq::message_t reply;

            send(recon_->previewData());
            spdlog::info("Volume preview data sent");

            for (auto& slice_data : recon_->sliceData()) {
                int slice_id = slice_data.slice_id;
                send(slice_data);
                spdlog::info("Slice data {} sent", slice_id);
            }
        }
    });

    sub_thread_.detach();
    req_thread_.detach();
}

} // tomop::slicerecon