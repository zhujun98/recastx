/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include "recon/rpc_server.hpp"
#include "recon/application.hpp"

#include "common/config.hpp"
#include "common/utils.hpp"

namespace recastx::recon {

using namespace std::string_literals;

ControlService::ControlService(Application* app) : app_(app) {}

grpc::Status ControlService::StartAcquiring(grpc::ServerContext* /*context*/,
                                            const google::protobuf::Empty* /*req*/,
                                            google::protobuf::Empty* /*rep*/) {
    try {
        app_->startAcquiring();
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start acquiring!");
        // FIXME
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, e.what()};
    }
}

grpc::Status ControlService::StopAcquiring(grpc::ServerContext* /*context*/,
                                           const google::protobuf::Empty*  /*req*/,
                                           google::protobuf::Empty* /*rep*/) {
    try {
        app_->stopAcquiring();
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        spdlog::error("Failed to stop acquiring!");
        // FIXME
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, e.what()};
    }
}

grpc::Status ControlService::StartProcessing(grpc::ServerContext* /*context*/,
                                             const google::protobuf::Empty*  /*req*/,
                                             google::protobuf::Empty* /*rep*/) {
    try {
        app_->startProcessing();
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start processing!");
        // FIXME
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, e.what()};
    }
}

grpc::Status ControlService::StopProcessing(grpc::ServerContext* /*context*/,
                                            const google::protobuf::Empty*  /*req*/,
                                            google::protobuf::Empty* /*rep*/) {
    try {
        app_->stopProcessing();
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        spdlog::error("Failed to stop processing!");
        // FIXME
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, e.what()};
    }
}

grpc::Status ControlService::GetServerState(grpc::ServerContext* /*context*/,
                                            const google::protobuf::Empty* /*req*/,
                                            rpc::ServerState* state) {
    state->set_state(app_->getServerState());
    return grpc::Status::OK;
}

grpc::Status ControlService::SetScanMode(grpc::ServerContext* /*context*/,
                                         const rpc::ScanMode* mode,
                                         google::protobuf::Empty* /*rep*/) {
    app_->setScanMode(mode->mode(), mode->update_interval());
    return grpc::Status::OK;
}


ImageprocService::ImageprocService(Application* app) : app_(app) {}

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* /*context*/,
                                               const rpc::DownsamplingParams* params,
                                               google::protobuf::Empty* /*rep*/) {
    app_->setDownsampling(params->col(), params->row());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetCorrection(grpc::ServerContext* /*context*/,
                                             const rpc::CorrectionParams* params,
                                             google::protobuf::Empty* /*rep*/) {
    app_->setCorrection(params->offset(), params->minus_log());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* /*context*/,
                                             const rpc::RampFilterParams* params,
                                             google::protobuf::Empty* /*rep*/) {
    app_->setRampFilter(std::move(params->name()));
    return grpc::Status::OK;
}


ProjectionTransferService::ProjectionTransferService(Application* app) : app_(app) {}

grpc::Status ProjectionTransferService::SetProjectionGeometry(grpc::ServerContext* /*context*/,
                                                              const rpc::ProjectionGeometry* geometry,
                                                              google::protobuf::Empty* /*ack*/) {
    BeamShape beam_shape;
    if (geometry->beam_shape() == static_cast<int>(BeamShape::PARALELL)) beam_shape = BeamShape::PARALELL;
    else if (geometry->beam_shape() == static_cast<int>(BeamShape::CONE)) beam_shape = BeamShape::CONE;
    else {
      spdlog::error("Unknown beam shape: {}", geometry->beam_shape());
      return {grpc::StatusCode::INVALID_ARGUMENT, "Unknown beam shape"};
    }

    AngleRange angle_range;
    if (geometry->angle_range() == static_cast<int>(AngleRange::HALF)) angle_range = AngleRange::HALF;
    else if (geometry->beam_shape() == static_cast<int>(AngleRange::FULL)) angle_range = AngleRange::FULL;
    else {
        spdlog::error("Unknown angle range: {}", geometry->angle_range());
        return {grpc::StatusCode::INVALID_ARGUMENT, "Unknown angle range"};
    }

    app_->setProjectionGeometry(beam_shape,
                              geometry->col_count(), geometry->row_count(),
                              geometry->pixel_width(), geometry->pixel_height(),
                              geometry->src2origin(), geometry->origin2det(),
                              geometry->angle_count(), angle_range);
    return grpc::Status::OK;
}

grpc::Status ProjectionTransferService::SetProjection(grpc::ServerContext* /*context*/,
                                                      const rpc::Projection* request,
                                                      google::protobuf::Empty* /*rep*/) {
    app_->setProjectionReq(request->id());
    return grpc::Status::OK;
}

grpc::Status ProjectionTransferService::GetProjectionData(
        grpc::ServerContext* /*context*/,
        const google::protobuf::Empty*,
        grpc::ServerWriter<rpc::ProjectionData>* writer) {
    auto proj = app_->getProjectionData(100);
    if (proj) {
        writer->Write(proj.value());
        spdlog::debug("Projection data sent");
    }

    return grpc::Status::OK;
}


ReconstructionService::ReconstructionService(Application* app) : app_(app) {}

grpc::Status ReconstructionService::SetReconGeometry(grpc::ServerContext* /*context*/,
                                                     const rpc::ReconGeometry* geometry,
                                                     google::protobuf::Empty* /*ack*/) {
    std::optional<uint32_t> slice_size;
    if (geometry->slice_size()[0] != 0) slice_size = geometry->slice_size()[0];

    std::optional<uint32_t> volume_size;
    if (geometry->volume_size()[0] != 0) volume_size = geometry->volume_size()[0];

    auto parseRange = [](auto vrange)
            -> std::pair<std::optional<float>, std::optional<float>> {
        std::optional<float> v_min;
        std::optional<float> v_max;
        if (vrange[0] < vrange[1]) {
            v_min = static_cast<float>(vrange[0]);
            v_max = static_cast<float>(vrange[1]);
        }

        return {v_min, v_max};
    };

    auto [x_min, x_max] = parseRange(geometry->x_range());
    auto [y_min, y_max] = parseRange(geometry->y_range());
    auto [z_min, z_max] = parseRange(geometry->z_range());

    app_->setReconGeometry(slice_size, volume_size, x_min, x_max, y_min, y_max, z_min, z_max);
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* /*context*/,
                                             const rpc::Slice* slice,
                                             google::protobuf::Empty* /*rep*/) {
    Orientation orient;
    std::copy(slice->orientation().begin(), slice->orientation().end(), orient.begin());
    app_->setSliceReq(slice->timestamp(), orient);
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::SetVolume(grpc::ServerContext* /*context*/,
                                              const rpc::Volume* volume,
                                              google::protobuf::Empty* /*rep*/) {

    app_->setVolumeReq(volume->required());
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* /*context*/,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<rpc::ReconData>* writer) {
    // - Do not block because slice request needs to be responsive
    // - If the number of the logical threads are more than the number of the physical threads, 
    //   the volume_data could always have value.
    auto volume_data = app_->getVolumeData(0);
    if (!volume_data.empty()) {
        auto slice_data = app_->getSliceData(-1);

        if (app_->hasVolume()) {
            for (const auto& item : volume_data) {
                writer->Write(item);
            }
            spdlog::debug("Volume data sent");
        }
        
        for (const auto& item : slice_data) {
            writer->Write(item);
            auto ts = item.slice().timestamp();
            spdlog::debug("Slice data {} ({}) sent", sliceIdFromTimestamp(ts), ts);
        }
    } else {
        auto slice_data = app_->getOnDemandSliceData(10);

        if (!slice_data.empty()) {
            for (const auto& item : slice_data) {
                writer->Write(item);
                auto ts = item.slice().timestamp();
                spdlog::debug("On-demand slice data {} ({}) sent", sliceIdFromTimestamp(ts), ts);
            }
        }
    }

    return grpc::Status::OK;
}

RpcServer::RpcServer(int port, Application* app)
        : address_("0.0.0.0:" + std::to_string(port)), 
    control_service_(app),
    imageproc_service_(app),
    proj_trans_service_(app),
    recon_service_(app)  {
}

void RpcServer::start() {

    spdlog::info("Starting RPC services at {}", address_);
    thread_ = std::thread([&] {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder.RegisterService(&control_service_);
        builder.RegisterService(&imageproc_service_);
        builder.RegisterService(&proj_trans_service_);
        builder.RegisterService(&recon_service_);
 
        builder.SetMaxSendMessageSize(K_MAX_RPC_SERVER_SEND_MESSAGE_SIZE);

        server_ = builder.BuildAndStart();
        server_->Wait();
    });

    thread_.detach();
}

} // namespace recastx::recon