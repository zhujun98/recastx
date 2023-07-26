/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_TEST_RPCSERVER_HPP
#define GUI_TEST_RPCSERVER_HPP

#include <array>
#include <random>
#include <string>
#include <thread>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "projection.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

#include "common/config.hpp"

namespace recastx::gui::test {

inline constexpr size_t K_RECON_INTERVAL = 200;

inline std::vector<float> generateRandomVec(size_t s, float min_v = 0.f, float max_v = 1.f) {
    std::vector<float> vec(s);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min_v, max_v);
    auto f = [&] { return dist(gen); };
    std::generate(vec.begin(), vec.end(), f);
    return vec;
}

class RpcServer;

class ControlService final : public Control::Service {

    ServerState_State state_;

    RpcServer* server_;

  public:

    explicit ControlService(RpcServer* server);

    grpc::Status SetServerState(grpc::ServerContext* context,
                                const ServerState* state,
                                google::protobuf::Empty* ack) override;

    grpc::Status SetScanMode(grpc::ServerContext* context,
                             const ScanMode* mode,
                             google::protobuf::Empty* ack) override;

    void updateState(ServerState_State state) { state_ = state; }
};

class ImageprocService final : public Imageproc::Service {

    ServerState_State state_;

  public:

    grpc::Status SetDownsampling(grpc::ServerContext* context,
                                 const DownsamplingParams* params,
                                 google::protobuf::Empty* ack) override;

    grpc::Status SetRampFilter(grpc::ServerContext* contest,
                               const RampFilterParams* params,
                               google::protobuf::Empty* ack) override;

    void updateState(ServerState_State state) { state_ = state; }
};

class ProjectionService final : public rpc::Projection::Service {

    ServerState_State state_;

    void setProjectionData(rpc::ProjectionData* data);

  public:

    grpc::Status GetProjectionData(grpc::ServerContext* context,
                                   const google::protobuf::Empty*,
                                   grpc::ServerWriter<rpc::ProjectionData>* writer) override;

    void updateState(ServerState_State state) { state_ = state; }
};

class ReconstructionService final : public Reconstruction::Service {

    ServerState_State state_;

    std::thread thread_;

    uint64_t timestamp_ {0};
    std::array<uint64_t, MAX_NUM_SLICES> timestamps_;

    void setSliceData(ReconData* data, size_t id);

    void setVolumeData(ReconData* data);

  public:

    ReconstructionService();

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const Slice* slice,
                          google::protobuf::Empty* ack) override;

    grpc::Status GetReconData(grpc::ServerContext* context,
                              const google::protobuf::Empty*,
                              grpc::ServerWriter<ReconData>* writer) override;

    void updateState(ServerState_State state) { state_ = state; }
};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ProjectionService projection_service_;
    ReconstructionService reconstruction_service_;

  public:

    explicit RpcServer(int port);

    void start();

    void updateState(ServerState_State state);
};


} // recastx::gui::test

#endif //GUI_TEST_RPCSERVER_HPP
