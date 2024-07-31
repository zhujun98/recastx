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

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* /*context*/,
                                               const rpc::DownsamplingParams* params,
                                               google::protobuf::Empty* /*ack*/) {
    spdlog::info("Set projection downsampling: {} / {}", params->col(), params->row());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetCorrection(grpc::ServerContext* /*context*/,
                                             const rpc::CorrectionParams* params,
                                             google::protobuf::Empty* /*ack*/) {
    spdlog::info("Set projection center corrections: {}", params->offset());
    spdlog::info("Set minus log: {}", params->minus_log() ? "enabled" : "disabled");
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* /*contest*/,
                                             const rpc::RampFilterParams* params,
                                             google::protobuf::Empty* /*ack*/) {
    spdlog::info("Set projection filter: {}", params->name());
    return grpc::Status::OK;
}

ProjectionTransferService::ProjectionTransferService(Application* app) : app_(app) {}

grpc::Status ProjectionTransferService::SetProjectionGeometry(grpc::ServerContext* /*context*/,
                                                              const rpc::ProjectionGeometry* geometry,
                                                              google::protobuf::Empty* /*ack*/) {
  spdlog::info("Set projection geometry");
  spdlog::info(" - Beam shape: {}", geometry->beam_shape() == 0 ? "PARALLEL" : "CONE");
  spdlog::info(" - Col count / Row count: {} / {}", geometry->col_count(), geometry->row_count());
  x_ = geometry->col_count();
  y_ = geometry->row_count();
  spdlog::info(" - Pixel width / Pixel height: {} / {}", geometry->pixel_width(), geometry->pixel_height());
  spdlog::info(" - Source to origin / Origin to detector: {} / {}", geometry->src2origin(), geometry->origin2det());
  spdlog::info(" - Angle count / Angle range: {} / {}", geometry->angle_count(), geometry->angle_range() == 0 ? "HALF" : "FULL");
  return grpc::Status::OK;
}

grpc::Status ProjectionTransferService::SetProjection(grpc::ServerContext* /*context*/,
                                                      const rpc::Projection* request,
                                                      google::protobuf::Empty* /*ack*/) {
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
    int x = 1024;
    int y = 1024;
    if (x_ != 0 && y_ != 0) {
        x = x_;
        y = y_;
    }
    auto vec = generateRandomRawData(x * y, 0, 1000 + 10 * proj_id_);
    data->set_id(proj_id_);
    data->set_col_count(x);
    data->set_row_count(y);
    data->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
}


ReconstructionService::ReconstructionService(Application* app) : app_(app), timestamps_ {0, 1, 2} {}

grpc::Status ReconstructionService::SetReconGeometry(grpc::ServerContext* context,
                                                     const rpc::ReconGeometry* geometry,
                                                     google::protobuf::Empty* ack) {
    spdlog::info("Set reconstruction geometry");

    slice_x_ = geometry->slice_size()[0];
    slice_y_ = geometry->slice_size()[1];
    spdlog::info(" - Slice size: {} x {}", slice_x_, slice_y_);

    volume_x_ = geometry->volume_size()[0];
    volume_y_ = geometry->volume_size()[1];
    volume_z_ = geometry->volume_size()[2];
    spdlog::info(" - Volume size: {} x {} x {}", volume_x_, volume_y_, volume_z_);

    spdlog::info(" - X range: {} - {}", geometry->x_range()[0], geometry->x_range()[1]);
    spdlog::info(" - Y range: {} - {}", geometry->y_range()[0], geometry->y_range()[1]);
    spdlog::info(" - Z range: {} - {}", geometry->z_range()[0], geometry->z_range()[1]);
    return grpc::Status::OK;
}

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

        {
            uint32_t x = 128;
            uint32_t y = 128;
            uint32_t z = 128;
            if (volume_x_ != 0 && volume_y_ != 0 && volume_z_ != 0) {
                x = volume_x_;
                y = volume_y_;
                z = volume_z_;
            }
            for (uint32_t i = 0; i < z; ++i) {
                setVolumeShardData(&data, x, y, z, i);
                writer->Write(data);
            }
            spdlog::info("Volume data sent");
        }


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
    uint32_t x = 1024;
    uint32_t y = 1024;
    if (slice_x_ != 0 && slice_y_ != 0) {
        x = slice_x_;
        y = slice_y_;
    }

    auto slice = data->mutable_slice();

    auto vec = generateRandomProcData(
            x * y, 0.f, 1.f + (id >= 0 ? (float)id : (float)sliceIdFromTimestamp(timestamp_)));
    slice->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
    slice->set_col_count(x);
    slice->set_row_count(y);
    if (id >= 0) {
        slice->set_timestamp(timestamps_.at(id));
    } else {
        slice->set_timestamp(timestamp_);
    }
}

void ReconstructionService::setVolumeShardData(
        rpc::ReconData* data, uint32_t x, uint32_t y, uint32_t z, uint32_t shard_id) {
    auto shard = data->mutable_volume_shard();

    auto vec = generateRandomProcData(x * y, 0.f, 0.5f);
    shard->set_data(vec.data(), vec.size() * sizeof(float));
    shard->set_col_count(x);
    shard->set_row_count(y);
    shard->set_slice_count(z);
    shard->set_pos(shard_id * x * y);
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