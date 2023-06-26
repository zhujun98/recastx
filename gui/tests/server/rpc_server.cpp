#include <random>

#include <spdlog/spdlog.h>

#include "common/utils.hpp"
#include "rpc_server.hpp"

namespace recastx::gui::test {

using namespace std::string_literals;
using namespace std::chrono_literals;

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
    spdlog::info("Update slice parameters: {} ({})", sliceIdFromTimestamp(slice->timestamp()), slice->timestamp());
    timestamp_ = slice->timestamp();
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<ReconData>* writer) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 3);

    ReconData data;
    while (true) {
        int num = dist(gen);
        if (num == 0) continue;

        if (num == 3) {
            setVolumeData(&data);
            writer->Write(data);
        }

        for (int i = 0; i < num; ++i) {
            setSliceData(&data);
            writer->Write(data);
        }

        std::this_thread::sleep_for(100ms);
        break;
    }

    return grpc::Status::OK;
}

void ReconstructionService::setSliceData(ReconData* data) {
    auto slice = data->mutable_slice();

    std::vector<float> vec(1024 * 2048);
    slice->set_data(vec.data(), vec.size() * sizeof(float));
    slice->set_col_count(1024);
    slice->set_row_count(2048);
    slice->set_timestamp(timestamp_);
}

void ReconstructionService::setVolumeData(ReconData* data) {
    auto vol = data->mutable_volume();

    std::vector<float> vec(128 * 128 * 128);
    vol->set_data(vec.data(), vec.size() * sizeof(float));
    vol->set_col_count(128);
    vol->set_row_count(128);
    vol->set_slice_count(128);
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