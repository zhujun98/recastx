/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <thread>

#include "common/config.hpp"
#include "rpc_client.hpp"
#include "logger.hpp"

namespace recastx::gui {

using namespace std::string_literals;

std::queue<ReconData>& RpcClient::packets() { return packets_; }

RpcClient::RpcClient(const std::string& hostname, int port) {

    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(K_MAX_RPC_RECV_MESSAGE_SIZE);
    channel_ = grpc::CreateCustomChannel(hostname + ":"s + std::to_string(port),
                                         grpc::InsecureChannelCredentials(),
                                         ch_args);

    control_stub_ = Control::NewStub(channel_);
    imageproc_stub_ = Imageproc::NewStub(channel_);
    reconstruction_stub_ = Reconstruction::NewStub(channel_);
}

bool RpcClient::setServerState(ServerState_State state) {
    ServerState request;
    request.set_state(state);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = control_stub_->SetServerState(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setScanMode(ScanMode_Mode mode, uint32_t update_interval) {
    ScanMode request;
    request.set_mode(mode);
    request.set_update_interval(update_interval);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = control_stub_->SetScanMode(&context, request, & reply);
    return checkStatus(status);
}

bool RpcClient::setDownsamplingParams(uint32_t col, uint32_t row) {
    DownsamplingParams request;
    request.set_col(col);
    request.set_row(row);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetDownsamplingParams(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setSlice(uint64_t timestamp, const Orientation& orientation) {
    Slice request;
    request.set_timestamp(timestamp);
    for (auto v : orientation) request.add_orientation(v);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = reconstruction_stub_->SetSlice(&context, request, &reply);
    return checkStatus(status);
}

void RpcClient::startReconDataStream() {
    thread_ = std::thread([&]() {
        streaming_ = true;
        ReconData reply;

        constexpr int min_timeout = 100;
        constexpr int max_timeout = 2000;
        int timeout = min_timeout;
        while (streaming_) {
            grpc::ClientContext context;

            google::protobuf::Empty request;

            std::unique_ptr<grpc::ClientReader<ReconData> > reader(
                    reconstruction_stub_->GetReconData(&context, request));
            while(reader->Read(&reply)) {
                log::debug("Received ReconData");
                packets_.push(reply);
            }
            grpc::Status status = reader->Finish();

            if (checkStatus(status)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
                timeout = std::min(2 * timeout, max_timeout);
            } else {
                timeout = min_timeout;
            }
        }

        log::debug("ReconData streaming finished");
    });
}

void RpcClient::stopReconDataStream() {
    streaming_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool RpcClient::checkStatus(const grpc::Status& status) const {
    if (!status.ok()) {
        log::debug("{}: {}", status.error_code(), status.error_message());
        log::warn("Failed to connect to the reconstruction server!");
        return true;
    }
    return false;
}

} // namespace recastx::gui
