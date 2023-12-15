/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "common/utils.hpp"
#include "rpc_server.hpp"

namespace recastx::gui::test {

using namespace std::string_literals;
using namespace std::chrono_literals;

ControlService::ControlService(RpcServer* server) : server_(server) {}

grpc::Status ControlService::SetServerState(grpc::ServerContext* context,
                                            const rpc::ServerState* state,
                                            google::protobuf::Empty* ack) {
    state_ = state->state();
    if (state_ == rpc::ServerState_State_PROCESSING) {
        spdlog::info("Start acquiring & processing data");
    } else if (state_ == rpc::ServerState_State_ACQUIRING) {
        spdlog::info("Start acquiring data");
    } else if (state_ == rpc::ServerState_State_READY) {
        spdlog::info("Stop acquiring & processing data");
    }
    server_->updateState(state_);
    return grpc::Status::OK;
}

grpc::Status ControlService::SetScanMode(grpc::ServerContext* context,
                                         const rpc::ScanMode* mode,
                                         google::protobuf::Empty* ack) {
    std::string mode_str;
    if (mode->mode() == rpc::ScanMode_Mode_CONTINUOUS) {
        mode_str = "continuous";
    } else if (mode->mode() == rpc::ScanMode_Mode_DISCRETE) {
        mode_str = "discrete";
    }
    spdlog::info("Set scan mode: {} / {}", mode_str, mode->update_interval());

    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* context,
                                               const rpc::DownsamplingParams* params,
                                               google::protobuf::Empty* ack) {
    spdlog::info("Set projection downsampling: {} / {}", params->col(), params->row());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* contest,
                                             const rpc::RampFilterParams* params,
                                             google::protobuf::Empty* ack) {
    spdlog::info("Set projection filter: {}", params->name());
    return grpc::Status::OK;
}

grpc::Status ProjectionTransferService::SetProjection(grpc::ServerContext* context,
                                                      const rpc::Projection* request,
                                                      google::protobuf::Empty* ack) {
    proj_id_ = request->id();
    spdlog::info("Set projection id: {}", proj_id_);
    return grpc::Status::OK;
}

grpc::Status ProjectionTransferService::GetProjectionData(grpc::ServerContext* context,
                                                          const google::protobuf::Empty*,
                                                          grpc::ServerWriter<rpc::ProjectionData>* writer) {
    if (state_ == rpc::ServerState_State_PROCESSING || state_ == rpc::ServerState_State_ACQUIRING) {
        if (state_ == rpc::ServerState_State_PROCESSING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kReconInterval));
        }

        rpc::ProjectionData data;
        spdlog::info("Set projection data");
        setProjectionData(&data);
        writer->Write(data);
    }

    return grpc::Status::OK;
}

void ProjectionTransferService::setProjectionData(rpc::ProjectionData *data) {
    auto vec = generateRandomRawData(x_ * y_, 0, 1000 + 10 * proj_id_);
    data->set_id(proj_id_);
    data->set_col_count(x_);
    data->set_row_count(y_);
    data->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
}

ReconstructionService::ReconstructionService() : timestamps_ {0, 1, 2} {}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context,
                                             const rpc::Slice* slice,
                                             google::protobuf::Empty* ack) {
    uint64_t ts = slice->timestamp();
    size_t id = sliceIdFromTimestamp(ts);
    spdlog::info("Update slice parameters: {} ({})", id, ts);

    timestamp_ = ts;
    on_demand_ = true;

    timestamps_.at(id) = ts;
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::SetVolume(grpc::ServerContext* context,
                                              const rpc::Volume* volume,
                                              google::protobuf::Empty* ack) {
    spdlog::info("Set volume required: {}", volume->required());
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<rpc::ReconData>* writer) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kReconInterval));

    rpc::ReconData data;
    if (state_ == rpc::ServerState_State_PROCESSING) {
        sino_uploaded_ = true;

        for (auto i = 0; i < volume_z_; ++i) {
            setVolumeShardData(&data, i);
            writer->Write(data);
        }
        spdlog::info("Volume data sent");

        for (int i = 0; i < timestamps_.size(); ++i) {
            setSliceData(&data, i);
            writer->Write(data);
            if (timestamps_.at(i) == timestamp_) on_demand_ = false;
        }
        spdlog::info("Slice data sent");
    }

    if (on_demand_ && sino_uploaded_) {
        setSliceData(&data, -1);
        writer->Write(data);
        spdlog::info("On-demand slice data: {} sent", timestamp_);
        on_demand_ = false;
    }
    return grpc::Status::OK;
}

void ReconstructionService::setSliceData(rpc::ReconData* data, int id) {
    auto slice = data->mutable_slice();

    auto vec = generateRandomProcData(
            slice_x_ * slice_y_, 0.f, 1.f + (id >= 0 ? (float)id : (float)sliceIdFromTimestamp(timestamp_)));
    slice->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
    slice->set_col_count(slice_x_);
    slice->set_row_count(slice_y_);
    if (id >= 0) {
        slice->set_timestamp(timestamps_.at(id));
    } else {
        slice->set_timestamp(timestamp_);
    }
}

void ReconstructionService::setVolumeShardData(rpc::ReconData* data, uint32_t index) {
    auto shard = data->mutable_volume_shard();

    auto vec = generateRandomProcData(volume_x_ * volume_y_, 0.f, 0.5f);
    shard->set_data(vec.data(), vec.size() * sizeof(float));
    shard->set_col_count(volume_x_);
    shard->set_row_count(volume_y_);
    shard->set_slice_count(volume_z_);
    shard->set_pos(index * volume_x_ * volume_y_);
}

RpcServer::RpcServer(int port)
        : address_("0.0.0.0:" + std::to_string(port)),
          control_service_(this) {
}

void RpcServer::start() {
    spdlog::info("Starting RPC services at {}", address_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&control_service_);
    builder.RegisterService(&imageproc_service_);
    builder.RegisterService(&proj_trans_service_);
    builder.RegisterService(&reconstruction_service_);

    server_ = builder.BuildAndStart();
    server_->Wait();
}

void RpcServer::updateState(rpc::ServerState_State state) {
    imageproc_service_.updateState(state);
    proj_trans_service_.updateState(state);
    reconstruction_service_.updateState(state);
}

} // namespace recastx::gui::test