#include <spdlog/spdlog.h>

#include "slicerecon/broker.hpp"


namespace slicerecon {

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

    std::vector<tomop::packet_desc> descriptors = {
        tomop::packet_desc::set_slice,
        tomop::packet_desc::remove_slice
    };

    for (auto descriptor : descriptors) {
        int32_t filter[] = {
            (std::underlying_type<tomop::packet_desc>::type)descriptor, 0
        };
        sub_socket_.set(zmq::sockopt::subscribe, zmq::buffer(filter));
    }
}

Broker::~Broker() {
    req_socket_.close();
    sub_socket_.close();
    context_.close();
}

void Broker::send(const tomop::Packet& packet) {
    std::lock_guard<std::mutex> guard(socket_mutex_);

    zmq::message_t reply;
    packet.send(req_socket_);
    req_socket_.recv(reply, zmq::recv_flags::none);
}

void Broker::start() {
    thread_ = std::thread([&] {
        while (true) {
            zmq::message_t update;
            sub_socket_.recv(update, zmq::recv_flags::none);
            auto desc = ((tomop::packet_desc*)update.data())[0];
            auto buffer = tomop::memory_buffer(update.size(), (char*)update.data());

            switch (desc) {
                case tomop::packet_desc::set_slice: {
                    auto packet = std::make_unique<tomop::SetSlicePacket>();
                    packet->deserialize(std::move(buffer));

                    makeSlice(packet->slice_id, packet->orientation);
                    break;
                }
                case tomop::packet_desc::remove_slice: {
                    auto packet = std::make_unique<tomop::RemoveSlicePacket>();
                    packet->deserialize(std::move(buffer));

                    auto to_erase = std::find_if(
                        slices_.begin(), slices_.end(), [&](auto x) {
                            return x.first == packet->slice_id;
                        });
                    slices_.erase(to_erase);

                    break;
                }
                default: {
                    spdlog::warn("Unrecognized package with descriptor {0:x}", 
                                 std::underlying_type<tomop::packet_desc>::type(desc));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    break;
                }
            }
        }
    });

    preview_thread_ = std::thread([&] {
        while (true) {

            zmq::message_t reply;

            int n = recon_->previewSize();
            auto volprev = tomop::VolumeDataPacket({n, n, n}, recon_->previewData());
            send(volprev);
            spdlog::info("Volume preview data sent");

            auto grsp = tomop::GroupRequestSlicesPacket(1);
            send(grsp);
        }
    });

    thread_.detach();
    preview_thread_.detach();
}

void Broker::makeSlice(int32_t slice_id, const std::array<float, 9>& orientation) {
    int32_t update_slice_index = -1;
    int32_t i = 0;
    for (auto& id_and_slice : slices_) {
        if (id_and_slice.first == slice_id) {
            update_slice_index = i;
            break;
        }
        ++i;
    }

    if (update_slice_index >= 0) {
        slices_[update_slice_index] = std::make_pair(slice_id, orientation);
    } else {
        slices_.push_back(std::make_pair(slice_id, orientation));
    }

    auto result = recon_->reconstructSlice(orientation);

    if (!result.first.empty()) {
        auto data_packet = tomop::SliceDataPacket(
            slice_id, result.first, std::move(result.second), false);
        send(data_packet);
    }
}

} // namespace slicerecon