#include <thread>

#include "rpc_client.hpp"

namespace recastx::gui {

using namespace std::string_literals;

//
//void DataClient::start() {
//    thread_ = std::thread([&]() {
//        while (true) {
//            zmq::message_t reply;
//
//            try {
//                socket_.send(zmq::str_buffer("GUIReady"), zmq::send_flags::none);
//                [[maybe_unused]] auto recv_ret = socket_.recv(reply, zmq::recv_flags::none);
//            } catch (const zmq::error_t& e) {
//                if (e.num() != ETERM) throw;
//                break;
//            }
//
//            ReconDataPacket packet;
//            packet.ParseFromArray(reply.data(), static_cast<int>(reply.size()));
//            packets_.push(std::move(packet));
//        }
//    });
//
//    thread_.detach();
//}
//
std::queue<ReconData>& RpcClient::packets() { return packets_; }

RpcClient::RpcClient(const std::string& hostname, int port)
        : channel_(grpc::CreateChannel(hostname + ":"s + std::to_string(port),
                                       grpc::InsecureChannelCredentials())),
          control_stub_(Control::NewStub(channel_)),
          imageproc_stub_(Imageproc::NewStub(channel_)),
          reconstruction_stub_(Reconstruction::NewStub(channel_)) {
}

void RpcClient::SetServerState(ServerState_State state) {
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

void RpcClient::SetDownsamplingParams(uint32_t downsampling_col, uint32_t downsampling_row) {
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

void RpcClient::SetSlice(uint64_t timestamp, const Orientation& orientation) {
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

void RpcClient::GetSliceData() {
    grpc::ClientContext context;

    google::protobuf::Empty request;
    std::unique_ptr<grpc::ClientReader<ReconData> > reader(
            reconstruction_stub_->GetReconData(&context, request));

    ReconData data;
    while (reader->Read(&data)) {
        packets_.push(data);
    }
}

} // namespace recastx::gui
