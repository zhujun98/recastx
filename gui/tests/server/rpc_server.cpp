#include <spdlog/spdlog.h>

#include "rpc_server.hpp"


namespace recastx::gui::test {

using namespace std::string_literals;


grpc::Status ControlService::SetServerState(grpc::ServerContext* context,
                                            const ServerState* state,
                                            google::protobuf::Empty* ack) {
    if (state->state() == ServerState_State::ServerState_State_PROCESSING) {
        spdlog::info("Start acquiring & processing ...");
    } else if (state->state() == ServerState_State::ServerState_State_ACQUIRING) {
        spdlog::info("Start acquiring ...");
    } else if (state->state() == ServerState_State::ServerState_State_READY) {
        spdlog::info("Stop acquiring & processing ...");
    }
    return grpc::Status::OK;
}


grpc::Status ImageprocService::SetDownsamplingParams(grpc::ServerContext* context,
                                                     const DownsamplingParams* params,
                                                     google::protobuf::Empty* ack) {
    spdlog::info("Set downsampling parameters: {} / {}", params->downsampling_row(), params->downsampling_col());
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context,
                                             const Slice* slice,
                                             google::protobuf::Empty* ack) {
    spdlog::info("Set slice: {}", slice->timestamp());
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<ReconData>* writer) {

    return grpc::Status::OK;
}

RpcServer::RpcServer(int port)
        : address_("0.0.0.0:" + std::to_string(port)) {
}

void RpcServer::start() {
    spdlog::info("Starting RPC services at {}", address_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&control_service_);
    builder.RegisterService(&imageproc_service_);
    builder.RegisterService(&reconstruction_service_);

    server_ = builder.BuildAndStart();
    server_->Wait();
}

} // namespace recastx::gui::test