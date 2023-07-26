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
                                            const ServerState* state,
                                            google::protobuf::Empty* ack) {
    state_ = state->state();
    if (state_ == ServerState_State::ServerState_State_PROCESSING) {
        spdlog::info("Start acquiring & processing data");
    } else if (state_ == ServerState_State::ServerState_State_ACQUIRING) {
        spdlog::info("Start acquiring data");
    } else if (state_ == ServerState_State::ServerState_State_READY) {
        spdlog::info("Stop acquiring & processing data");
    }
    server_->updateState(state_);
    return grpc::Status::OK;
}

grpc::Status ControlService::SetScanMode(grpc::ServerContext* context,
                                         const ScanMode* mode,
                                         google::protobuf::Empty* ack) {
    std::string mode_str;
    if (mode->mode() == ScanMode_Mode_CONTINUOUS) {
        mode_str = "continuous";
    } else if (mode->mode() == ScanMode_Mode_DISCRETE) {
        mode_str = "discrete";
    }
    spdlog::info("Set scan mode: {} / {}", mode_str, mode->update_interval());

    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetDownsampling(grpc::ServerContext* context,
                                               const DownsamplingParams* params,
                                               google::protobuf::Empty* ack) {
    spdlog::info("Set projection downsampling: {} / {}", params->col(), params->row());
    return grpc::Status::OK;
}

grpc::Status ImageprocService::SetRampFilter(grpc::ServerContext* contest,
                                             const RampFilterParams* params,
                                             google::protobuf::Empty* ack) {
    spdlog::info("Set projection filter: {}", params->name());
    return grpc::Status::OK;
}

grpc::Status ProjectionService::GetProjectionData(grpc::ServerContext* context,
                                                  const google::protobuf::Empty*,
                                                  grpc::ServerWriter<rpc::ProjectionData>* writer) {
    if (state_ == ServerState_State::ServerState_State_PROCESSING
            || state_ == ServerState_State::ServerState_State_ACQUIRING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(K_RECON_INTERVAL));

        rpc::ProjectionData data;
        spdlog::info("Set projection data");
        setProjectionData(&data);
        writer->Write(data);
    }

    return grpc::Status::OK;
}

void ProjectionService::setProjectionData(rpc::ProjectionData *data) {
    auto vec = generateRandomVec(1024 * 512, 0.f, 10.f);
    data->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
    data->set_col_count(1024);
    data->set_row_count(512);
}

ReconstructionService::ReconstructionService() : timestamps_ {0, 1, 2} {}

grpc::Status ReconstructionService::SetSlice(grpc::ServerContext* context,
                                             const Slice* slice,
                                             google::protobuf::Empty* ack) {
    uint64_t ts = slice->timestamp();
    size_t id = sliceIdFromTimestamp(ts);
    spdlog::info("Update slice parameters: {} ({})", id, ts);
    timestamps_.at(id) = ts;
    return grpc::Status::OK;
}

grpc::Status ReconstructionService::GetReconData(grpc::ServerContext* context,
                                                 const google::protobuf::Empty*,
                                                 grpc::ServerWriter<ReconData>* writer) {
    if (state_ == ServerState_State::ServerState_State_PROCESSING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(K_RECON_INTERVAL));

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 4);

        ReconData data;
        spdlog::info("Set volume data");
        setVolumeData(&data);
        writer->Write(data);

        for (int i = 0; i < timestamps_.size(); ++i) {
            setSliceData(&data, i);
            writer->Write(data);
        }

        if (dist(gen) == 4) {
            // simulate on-demand slice requested sporadically
            spdlog::info("Set on-demand slice data");
            setSliceData(&data, 0);
            writer->Write(data);
        }
    }

    return grpc::Status::OK;
}

void ReconstructionService::setSliceData(ReconData* data, size_t id) {
    auto slice = data->mutable_slice();

    auto vec = generateRandomVec(1024 * 512, 0.f, 1.f + id);
    slice->set_data(vec.data(), vec.size() * sizeof(decltype(vec)::value_type));
    slice->set_col_count(1024);
    slice->set_row_count(512);
    slice->set_timestamp(timestamps_.at(id));
}

void ReconstructionService::setVolumeData(ReconData* data) {
    auto vol = data->mutable_volume();

    auto vec = generateRandomVec(128 * 128 * 128, 0.f, 1.f + MAX_NUM_SLICES - 1);
    vol->set_data(vec.data(), vec.size() * sizeof(float));
    vol->set_col_count(128);
    vol->set_row_count(128);
    vol->set_slice_count(128);
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
    builder.RegisterService(&projection_service_);
    builder.RegisterService(&reconstruction_service_);

    server_ = builder.BuildAndStart();
    server_->Wait();
}

void RpcServer::updateState(ServerState_State state) {
    imageproc_service_.updateState(state);
    projection_service_.updateState(state);
    reconstruction_service_.updateState(state);
}

} // namespace recastx::gui::test