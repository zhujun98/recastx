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

std::queue<RpcClient::DataType>& RpcClient::packets() { return packets_; }

RpcClient::RpcClient(const std::string& address) {

    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(K_MAX_RPC_CLIENT_RECV_MESSAGE_SIZE);
    channel_ = grpc::CreateCustomChannel(
            address, grpc::InsecureChannelCredentials(), ch_args);

    control_stub_ = rpc::Control::NewStub(channel_);
    imageproc_stub_ = rpc::Imageproc::NewStub(channel_);
    proj_trans_stub_ = rpc::ProjectionTransfer::NewStub(channel_);
    recon_stub_ = rpc::Reconstruction::NewStub(channel_);
}

RpcClient::~RpcClient() {
    running_ = false;
    if (thread_proj_.joinable()) thread_proj_.join();
    if (thread_recon_.joinable()) thread_recon_.join();
}

bool RpcClient::setServerState(rpc::ServerState_State state) {
    rpc::ServerState request;
    request.set_state(state);

    google::protobuf::Empty reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->SetServerState(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setScanMode(rpc::ScanMode_Mode mode, uint32_t update_interval) {
    rpc::ScanMode request;
    request.set_mode(mode);
    request.set_update_interval(update_interval);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = control_stub_->SetScanMode(&context, request, & reply);
    return checkStatus(status);
}

bool RpcClient::setDownsampling(uint32_t col, uint32_t row) {
    rpc::DownsamplingParams request;
    request.set_col(col);
    request.set_row(row);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetDownsampling(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setRampFilter(const std::string& filter_name) {
    rpc::RampFilterParams request;
    request.set_name(filter_name);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetRampFilter(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setProjection(uint32_t id) {
    rpc::Projection request;
    request.set_id(id);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = proj_trans_stub_->SetProjection(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setSlice(uint64_t timestamp, const Orientation& orientation) {
    rpc::Slice request;
    request.set_timestamp(timestamp);
    for (auto v : orientation) request.add_orientation(v);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = recon_stub_->SetSlice(&context, request, &reply);
    return checkStatus(status);
}

bool RpcClient::setVolume(bool required) {
    rpc::Volume request;
    request.set_required(required);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = recon_stub_->SetVolume(&context, request, &reply);
    return checkStatus(status);
}

void RpcClient::start() {
    if (running_) return;

    running_ = true;
    startReadingReconStream();
    startReadingProjectionStream();
}

void RpcClient::toggleProjectionStream(bool state) {
    streaming_proj_.store(state, std::memory_order_release);
}

void RpcClient::startReadingReconStream() {
    thread_recon_ = std::thread([&]() {
        int timeout = min_timeout;

        google::protobuf::Empty request;
        rpc::ReconData reply;
        while (running_) {
            grpc::ClientContext context;
            std::unique_ptr<grpc::ClientReader<rpc::ReconData> > reader(
                    recon_stub_->GetReconData(&context, request));
            while(reader->Read(&reply)) {
                log::debug("Received ReconData");
                packets_.emplace(std::move(reply));
            }

            updateTimeout(timeout, reader->Finish());
        }

        log::debug("RPC ReconData streaming finished");
    });
}

void RpcClient::startReadingProjectionStream() {
    thread_proj_ = std::thread([&]() {
        int timeout = min_timeout;

        google::protobuf::Empty request;
        rpc::ProjectionData reply;
        while (running_) {
            if (!streaming_proj_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            grpc::ClientContext context;
            std::unique_ptr<grpc::ClientReader<rpc::ProjectionData> > reader(
                    proj_trans_stub_->GetProjectionData(&context, request));
            while(reader->Read(&reply)) {
                log::debug("Received ProjectionData");
                packets_.emplace(std::move(reply));
            }

            updateTimeout(timeout, reader->Finish());
        }

        log::debug("RPC ProjectionData streaming finished");
    });
}

bool RpcClient::checkStatus(const grpc::Status& status, bool warn_on_fail) const {
    if (!status.ok()) {
        log::debug("{}: {}", status.error_code(), status.error_message());
        if (warn_on_fail) {
            log::warn("Failed to connect to the reconstruction server!");
        }
        return true;
    }
    return false;
}

} // namespace recastx::gui
