#ifndef GUI_ZMQCLIENT_H
#define GUI_ZMQCLIENT_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

#include <zmq.hpp>

#include "logger.hpp"
#include "common/config.hpp"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::gui {

using namespace std::string_literals;

class DataClient {

    std::thread thread_;

    zmq::context_t context_;
    zmq::socket_t socket_;

    inline static std::queue<ReconDataPacket> packets_;

public:

    explicit DataClient(const std::string& hostname, int port);

    ~DataClient();

    static std::queue<ReconDataPacket>& packets();

    void start();
};

class RpcClient {

    std::shared_ptr<grpc::Channel> channel_;

    std::unique_ptr<Control::Stub> control_stub_;
    std::unique_ptr<Imageproc::Stub> imageproc_stub_;
    std::unique_ptr<Reconstruction::Stub> reconstruction_stub_;

  public:

    RpcClient(const std::string& hostname, int port)
        : channel_(grpc::CreateChannel(hostname + ":"s + std::to_string(port),
                                       grpc::InsecureChannelCredentials())),
          control_stub_(Control::NewStub(channel_)),
          imageproc_stub_(Imageproc::NewStub(channel_)),
          reconstruction_stub_(Reconstruction::NewStub(channel_)) {
    }

    void SetServerState(ServerState_State state) {
        ServerState request;
        request.set_state(state);

        google::protobuf::Empty reply;

        grpc::ClientContext context;
//        context.set_wait_for_ready(true);

        grpc::Status status = control_stub_->SetServerState(&context, request, &reply);

        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

    void SetDownsamplingParams(uint32_t downsampling_col, uint32_t downsampling_row) {
        DownsamplingParams request;
        request.set_downsampling_col(downsampling_col);
        request.set_downsampling_row(downsampling_row);

        google::protobuf::Empty reply;

        grpc::ClientContext context;
//        context.set_wait_for_ready(true);

        grpc::Status status = imageproc_stub_->SetDownsamplingParams(&context, request, &reply);

        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

    void SetSlice(uint64_t timestamp, const Orientation& orientation) {
        Slice request;
        request.set_timestamp(timestamp);
        for (auto v : orientation) request.add_orientation(v);

        google::protobuf::Empty reply;

        grpc::ClientContext context;
//        context.set_wait_for_ready(true);

        grpc::Status status = reconstruction_stub_->SetSlice(&context, request, &reply);

        if (!status.ok()) {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }
};

class MessageClient {

    zmq::context_t context_;
    zmq::socket_t socket_;

public:

    explicit MessageClient(const std::string& hostname, int port);

    ~MessageClient();

    template<typename T>
    void send(T&& packet);
};

template<typename T>
void MessageClient::send(T&& packet) {
    try {
        std::string encoded;
        packet.SerializeToString(&encoded);

        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);

        log::debug("Published packet");
    } catch (const std::exception& e) {
        log::error("Failed publishing packet: {}", e.what());
    }
}

}  // namespace recastx::gui

#endif //GUI_ZMQCLIENT_H