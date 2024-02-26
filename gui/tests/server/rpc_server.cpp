/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "common/utils.hpp"
#include "application.hpp"
#include "rpc_server.hpp"

namespace recastx::gui::test {

using namespace std::string_literals;
using namespace std::chrono_literals;

ControlService::ControlService(Application* app) : app_(app) {}

grpc::Status ControlService::StartAcquiring(grpc::ServerContext* /*context*/,
                                            const google::protobuf::Empty* /*req*/,
                                            google::protobuf::Empty* /*rep*/) {
    app_->startAcquiring();
    return grpc::Status::OK;
}

grpc::Status ControlService::StopAcquiring(grpc::ServerContext* /*context*/,
                                           const google::protobuf::Empty*  /*req*/,
                                           google::protobuf::Empty* /*rep*/) {
    app_->stopAcquiring();
    return grpc::Status::OK;
}

grpc::Status ControlService::StartProcessing(grpc::ServerContext* /*context*/,
                                             const google::protobuf::Empty*  /*req*/,
                                             google::protobuf::Empty* /*rep*/) {
    app_->startProcessing();
    return grpc::Status::OK;
}

grpc::Status ControlService::StopProcessing(grpc::ServerContext* /*context*/,
                                            const google::protobuf::Empty*  /*req*/,
                                            google::protobuf::Empty* /*rep*/) {
    app_->stopProcessing();
    return grpc::Status::OK;
}

grpc::Status ControlService::GetServerState(grpc::ServerContext* context,
                                            const google::protobuf::Empty* req,
                                            rpc::ServerState* state) {
    auto ret = app_->serverState();
    spdlog::info("Get server state: {}", ret);
    state->set_state(ret);
    return grpc::Status::OK;
}

grpc::Status ControlService::SetScanMode(grpc::ServerContext* context,
                                         const rpc::ScanMode* mode,
                                         google::protobuf::Empty* ack) {
    std::string mode_str;
    if (mode->mode() == rpc::ScanMode_Mode_CONTINUOUS) {
        mode_str = "continuous";
    } else if (mode->mode() == rpc::ScanMode_Mode_DYNAMIC) {
        mode_str = "dynamic";
    } else if (mode->mode() == rpc::ScanMode_Mode_STATIC) {
        mode_str = "static";
    }
    spdlog::info("Set scan mode: {} / {}", mode_str, mode->update_interval());

    return grpc::Status::OK;
}

ImageprocService::ImageprocService(Application* app) : app_(app) {}

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* context,
                                               const rpc::DownsamplingParams* params,
                                               google::protobuf::Empty* ack) {
    spdlog::info("Set projection downsampling: {} / {}", params->col(), params->row());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetCorrection(grpc::ServerContext* context,
                                             const rpc::CorrectionParams* params,
                                             google::protobuf::Empty* ack) {
    spdlog::info("Set projection corrections (H/V): {} / {}", params->col_offset(), params->row_offset());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* contest,
                                             const rpc::RampFilterParams* params,
                                             google::protobuf::Empty* ack) {
    spdlog::info("Set projection filter: {}", params->name());
    return grpc::Status::OK;
}

ProjectionTransferService::ProjectionTransferService(Application* app) : app_(app) {}

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
    auto state = app_->serverState();
    if (state & rpc::ServerState_State_PROCESSING) {
        if (state == rpc::ServerState_State_PROCESSING) {
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

ReconstructionService::ReconstructionService(Application* app) : app_(app), timestamps_ {0, 1, 2} {}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context,
                                             const rpc::Slice* slice,
                                             google::protobuf::Empty* ack) {
    uint64_t ts = slice->timestamp();
    size_t id = sliceIdFromTimestamp(ts);
    spdlog::info("Update slice parameters: {} ({})", id, ts);

    timestamp_ = ts;
    app_->setOnDemand(true);

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
    if (app_->serverState() == rpc::ServerState_State_PROCESSING) {
        app_->setSinoUploaded(true);

        for (auto i = 0; i < volume_z_; ++i) {
            setVolumeShardData(&data, i);
            writer->Write(data);
        }
        spdlog::info("Volume data sent");

        for (int i = 0; i < timestamps_.size(); ++i) {
            setSliceData(&data, i);
            writer->Write(data);
            if (timestamps_.at(i) == timestamp_) app_->setOnDemand(false);
        }
        spdlog::info("Slice data sent");
    }

    if (app_->onDemand() && app_->sinoUploaded()) {
        setSliceData(&data, -1);
        writer->Write(data);
        spdlog::info("On-demand slice data: {} sent", timestamp_);
        app_->setOnDemand(false);
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

RpcServer::RpcServer(int port, Application* app)
        : address_("0.0.0.0:" + std::to_string(port)),
          control_service_(app),
          imageproc_service_(app),
          proj_trans_service_(app),
          reconstruction_service_(app) {
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

} // namespace recastx::gui::test