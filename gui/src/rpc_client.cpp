/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <thread>

#include "common/config.hpp"
#include "rpc_client.hpp"
#include "logger.hpp"

namespace recastx::gui {

using namespace std::string_literals;

ThreadSafeQueue<RpcClient::DataType>& RpcClient::packets() { return packets_; }

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
    streaming_ = false;
    if (thread_proj_.joinable()) thread_proj_.join();
    if (thread_recon_.joinable()) thread_recon_.join();
}

RpcClient::State RpcClient::startAcquiring() {
    google::protobuf::Empty request;
    google::protobuf::Empty reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->StartAcquiring(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::stopAcquiring() {
    google::protobuf::Empty request;
    google::protobuf::Empty reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->StopAcquiring(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::startProcessing() {
    google::protobuf::Empty request;
    google::protobuf::Empty reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->StartProcessing(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::stopProcessing() {
    google::protobuf::Empty request;
    google::protobuf::Empty reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->StopProcessing(&context, request, &reply);
    return checkStatus(status);
}

std::optional<rpc::ServerState_State> RpcClient::getServerState() {
    google::protobuf::Empty request;

    rpc::ServerState reply;

    grpc::ClientContext context;
    grpc::Status status = control_stub_->GetServerState(&context, request, &reply);
    if (checkStatus(status) != State::OK) return std::nullopt;
    return reply.state();
}

RpcClient::State RpcClient::setScanMode(rpc::ScanMode_Mode mode, uint32_t update_interval) {
    rpc::ScanMode request;
    request.set_mode(mode);
    request.set_update_interval(update_interval);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = control_stub_->SetScanMode(&context, request, & reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setDownsampling(uint32_t col, uint32_t row) {
    rpc::DownsamplingParams request;
    request.set_col(col);
    request.set_row(row);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetDownsampling(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setCorrection(int32_t offset, bool minus_log) {
    rpc::CorrectionParams request;
    request.set_offset(offset);
    request.set_minus_log(minus_log);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetCorrection(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setRampFilter(const std::string& filter_name) {
    rpc::RampFilterParams request;
    request.set_name(filter_name);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = imageproc_stub_->SetRampFilter(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setProjectionGeometry(uint32_t beam_shape, uint32_t col_count, uint32_t row_count,
                                                  float pixel_width, float pixel_height,
                                                  float src2origin, float origin2det,
                                                  uint32_t angle_count, int angle_range) {
    rpc::ProjectionGeometry request;
    request.set_beam_shape(beam_shape);
    request.set_col_count(col_count);
    request.set_row_count(row_count);
    request.set_pixel_width(pixel_width);
    request.set_pixel_height(pixel_height);
    request.set_src2origin(src2origin);
    request.set_origin2det(origin2det);
    request.set_angle_count(angle_count);
    request.set_angle_range(angle_range);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = proj_trans_stub_->SetProjectionGeometry(&context, request, &reply);
    return checkStatus(status);
}


RpcClient::State RpcClient::setProjection(uint32_t id) {
    rpc::Projection request;
    request.set_id(id);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = proj_trans_stub_->SetProjection(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setReconGeometry(uint32_t slice_size,
                                             uint32_t volume_size,
                                             std::array<int32_t, 2> x,
                                             std::array<int32_t, 2> y,
                                             std::array<int32_t, 2> z) {
    rpc::ReconGeometry request;
    request.add_slice_size(slice_size);
    request.add_slice_size(slice_size);
    request.add_volume_size(volume_size);
    request.add_volume_size(volume_size);
    request.add_volume_size(volume_size);
    request.add_x_range(x[0]);
    request.add_x_range(x[1]);
    request.add_y_range(y[0]);
    request.add_y_range(y[1]);
    request.add_z_range(z[0]);
    request.add_z_range(z[1]);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = recon_stub_->SetReconGeometry(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setSlice(uint64_t timestamp, const Orientation& orientation) {
    rpc::Slice request;
    request.set_timestamp(timestamp);
    for (auto v : orientation) request.add_orientation(v);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = recon_stub_->SetSlice(&context, request, &reply);
    return checkStatus(status);
}

RpcClient::State RpcClient::setVolume(bool required) {
    rpc::Volume request;
    request.set_required(required);

    google::protobuf::Empty reply;

    grpc::ClientContext context;

    grpc::Status status = recon_stub_->SetVolume(&context, request, &reply);
    return checkStatus(status);
}

std::optional<rpc::ServerState_State> RpcClient::shakeHand() {
    auto state = getServerState();
    if (!state) return std::nullopt;
    return state.value();
}

void RpcClient::startStreaming() {
    if (streaming_) return;

    streaming_ = true;
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
        while (streaming_) {
            grpc::ClientContext context;
            std::unique_ptr<grpc::ClientReader<rpc::ReconData> > reader(
                    recon_stub_->GetReconData(&context, request));
            while(reader->Read(&reply)) {
                log::debug("Received ReconData");
                packets_.push(reply);
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
        while (streaming_) {
            if (!streaming_proj_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            grpc::ClientContext context;
            std::unique_ptr<grpc::ClientReader<rpc::ProjectionData> > reader(
                    proj_trans_stub_->GetProjectionData(&context, request));
            while(reader->Read(&reply)) {
                log::debug("Received ProjectionData");
                packets_.push(reply);
            }

            updateTimeout(timeout, reader->Finish());
        }

        log::debug("RPC ProjectionData streaming finished");
    });
}

RpcClient::State RpcClient::checkStatus(const grpc::Status& status, bool warn_on_unavailable_server) const {
    auto code = status.error_code();
    if (code == grpc::StatusCode::OK) return State::OK;

    const std::string& msg = status.error_message();
    if (code == grpc::StatusCode::UNAVAILABLE) {
        if (warn_on_unavailable_server) {
            log::warn("Reconstruction server not available!");
        }
    } else if (code == grpc::StatusCode::INVALID_ARGUMENT){
        log::error("Reconstruction server invalid argument: {}", msg);
    } else if (code == grpc::StatusCode::RESOURCE_EXHAUSTED){
        log::error("Reconstruction server resource exhausted: {}", msg);
    } else if (code == grpc::StatusCode::UNKNOWN) {
        log::error("Reconstruction server unknown error: {}", msg);
    } else {
        log::error("Unexpected RPC error {}: {}", code, msg);
        std::exit(EXIT_FAILURE);
    }
    return State::ERROR;
}

} // namespace recastx::gui
