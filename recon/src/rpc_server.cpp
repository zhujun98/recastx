/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

grpc::Status ControlService::SetServerState(grpc::ServerContext* context, 
                                            const rpc::ServerState* state,
                                            google::protobuf::Empty* ack) {
    app_->onStateChanged(state->state());
    return grpc::Status::OK;
}

grpc::Status ControlService::SetScanMode(grpc::ServerContext* context,
                                         const rpc::ScanMode* mode,
                                         google::protobuf::Empty* ack) {
    app_->setScanMode(mode->mode(), mode->update_interval());
    return grpc::Status::OK;
}


ImageprocService::ImageprocService(Application* app) : app_(app) {}

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* context, 
                                               const rpc::DownsamplingParams* params,
                                               google::protobuf::Empty* ack) {
    auto col = params->col();
    auto row = params->row();
    app_->setDownsampling(col, row);
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* contest,
                                             const rpc::RampFilterParams* params,
                                             google::protobuf::Empty* ack) {
    app_->setRampFilter(std::move(params->name()));
    return grpc::Status::OK;
}


ProjectionService::ProjectionService(Application* app) : app_(app) {}

grpc::Status ProjectionService::GetProjectionData(grpc::ServerContext* context,
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

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context, 
                                             const rpc::Slice* slice,
                                             google::protobuf::Empty* ack) {
    Orientation orient;
    std::copy(slice->orientation().begin(), slice->orientation().end(), orient.begin());
    app_->setSlice(slice->timestamp(), orient);
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::SetVolume(grpc::ServerContext* context,
                                              const rpc::Volume* volume,
                                              google::protobuf::Empty* ack) {

    app_->setVolume(volume->required());
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<rpc::ReconData>* writer) {
    // - Do not block because slice request needs to be responsive
    // - If the number of the logical threads are more than the number of the physical threads, 
    //   the preview_data could always have value.
    auto preview_data = app_->getVolumeData(0);
    if (preview_data) {
        auto slice_data = app_->getSliceData(-1);

        if (app_->hasVolume()) {
            writer->Write(preview_data.value());
            spdlog::debug("Preview data sent");
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
                spdlog::debug("On-demand slice data {} ({}) sent", 
                              sliceIdFromTimestamp(ts), ts);
            }
        }
    }

    return grpc::Status::OK;
}

RpcServer::RpcServer(int port, Application* app)
        : address_("0.0.0.0:" + std::to_string(port)), 
    control_service_(app),
    imageproc_service_(app),
    projection_service_(app),
    reconstruction_service_(app)  {
}

void RpcServer::start() {

    spdlog::info("Starting RPC services at {}", address_);
    thread_ = std::thread([&] {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder.RegisterService(&control_service_);
        builder.RegisterService(&imageproc_service_);
        builder.RegisterService(&projection_service_);
        builder.RegisterService(&reconstruction_service_);
 
        builder.SetMaxSendMessageSize(K_MAX_RPC_SERVER_SEND_MESSAGE_SIZE);

        server_ = builder.BuildAndStart();
        server_->Wait();
    });

    thread_.detach();
}

} // namespace recastx::recon