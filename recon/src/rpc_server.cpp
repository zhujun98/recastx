#include <spdlog/spdlog.h>

#include "recon/rpc_server.hpp"
#include "recon/application.hpp"

#include "common/utils.hpp"

namespace recastx::recon {

using namespace std::string_literals;

ControlService::ControlService(Application* app) : app_(app) {}

grpc::Status ControlService::SetServerState(grpc::ServerContext* context, 
                                            const ServerState* state,
                                            google::protobuf::Empty* ack) {
    app_->onStateChanged(state->state(), state->mode());
    return grpc::Status::OK;
}

ImageprocService::ImageprocService(Application* app) : app_(app) {}

grpc::Status ImageprocService::SetDownsamplingParams(grpc::ServerContext* context, 
                                                     const DownsamplingParams* params,
                                                     google::protobuf::Empty* ack) {
    auto col = params->col();
    auto row = params->row();
    app_->setDownsamplingParams(col, row);
    spdlog::info("Set image downsampling parameters: {} / {}", col, row);
    return grpc::Status::OK;
}

ReconstructionService::ReconstructionService(Application* app) : app_(app) {}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context, 
                                             const Slice* slice,
                                             google::protobuf::Empty* ack) {
    Orientation orient;
    std::copy(slice->orientation().begin(), slice->orientation().end(), orient.begin());
    app_->setSlice(slice->timestamp(), orient);
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<ReconData>* writer) {
    // - Do not block because slice request needs to be responsive
    // - If the number of the logical threads are more than the number of the physical threads, 
    //   the preview_data could always have value.
    auto preview_data = app_->previewData(0);
    if (preview_data) {
        auto slice_data = app_->sliceData(-1);

        writer->Write(preview_data.value());
        
        for (const auto& item : slice_data) {
            writer->Write(item);
            auto ts = item.slice().timestamp();
            spdlog::debug("Slice data {} ({}) sent", sliceIdFromTimestamp(ts), ts);
        }
    } else {
        auto slice_data = app_->onDemandSliceData(10);

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
          reconstruction_service_(app)  {
}

void RpcServer::start() {

    spdlog::info("Starting RPC services at {}", address_);
    thread_ = std::thread([&] {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder.RegisterService(&control_service_);
        builder.RegisterService(&imageproc_service_);
        builder.RegisterService(&reconstruction_service_);

        server_ = builder.BuildAndStart();
        server_->Wait();
    });

    thread_.detach();
}

} // namespace recastx::recon