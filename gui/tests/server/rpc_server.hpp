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

inline constexpr int kReconInterval = 200;

inline std::vector<ProDtype> generateRandomProcData(size_t s, ProDtype min_v = 0.f, ProDtype max_v = 1.f) {
    std::vector<ProDtype> vec(s);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<ProDtype> dist(min_v, max_v);
    auto f = [&] { return dist(gen); };
    std::generate(vec.begin(), vec.end(), f);
    return vec;
}

inline std::vector<RawDtype> generateRandomRawData(size_t s, RawDtype min_v, RawDtype max_v) {
    std::vector<RawDtype> vec(s);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<RawDtype> dist(min_v, max_v);
    auto f = [&] { return dist(gen); };
    std::generate(vec.begin(), vec.end(), f);
    return vec;
}

class RpcServer;

class ControlService final : public rpc::Control::Service {

    rpc::ServerState_State state_;

    RpcServer* server_;

  public:

    explicit ControlService(RpcServer* server);

    grpc::Status SetServerState(grpc::ServerContext* context,
                                const rpc::ServerState* state,
                                google::protobuf::Empty* ack) override;

    grpc::Status SetScanMode(grpc::ServerContext* context,
                             const rpc::ScanMode* mode,
                             google::protobuf::Empty* ack) override;

    void updateState(rpc::ServerState_State state) { state_ = state; }
};

class ImageprocService final : public rpc::Imageproc::Service {

    rpc::ServerState_State state_;

  public:

    grpc::Status SetDownsampling(grpc::ServerContext* context,
                                 const rpc::DownsamplingParams* params,
                                 google::protobuf::Empty* ack) override;

    grpc::Status SetRampFilter(grpc::ServerContext* contest,
                               const rpc::RampFilterParams* params,
                               google::protobuf::Empty* ack) override;

    void updateState(rpc::ServerState_State state) { state_ = state; }
};

class ProjectionTransferService final : public rpc::ProjectionTransfer::Service {

    uint32_t proj_id_;

    rpc::ServerState_State state_;

    void setProjectionData(rpc::ProjectionData* data);

  public:

    grpc::Status SetProjection(grpc::ServerContext* context,
                               const rpc::Projection* request,
                               google::protobuf::Empty* ack) override;

    grpc::Status GetProjectionData(grpc::ServerContext* context,
                                   const google::protobuf::Empty*,
                                   grpc::ServerWriter<rpc::ProjectionData>* writer) override;

    void updateState(rpc::ServerState_State state) { state_ = state; }
};

  class ReconstructionService final : public rpc::Reconstruction::Service {

    rpc::ServerState_State state_;
    bool sino_uploaded_ = false;

    std::thread thread_;

    bool on_demand_ = false;
    uint64_t timestamp_ {0};

    std::array<uint64_t, MAX_NUM_SLICES> timestamps_;

    void setSliceData(rpc::ReconData* data, int id);

    void setVolumeData(rpc::ReconData* data);

  public:

    ReconstructionService();

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const rpc::Slice* slice,
                          google::protobuf::Empty* ack) override;

    grpc::Status SetVolume(grpc::ServerContext* context,
                           const rpc::Volume* volume,
                           google::protobuf::Empty* ack) override;

    grpc::Status GetReconData(grpc::ServerContext* context,
                              const google::protobuf::Empty*,
                              grpc::ServerWriter<rpc::ReconData>* writer) override;

    void updateState(rpc::ServerState_State state) {
        if (state != state_) {
            if (state == rpc::ServerState_State_READY) {
                on_demand_ = false;
            } else if (state == rpc::ServerState_State_ACQUIRING) {
                sino_uploaded_ = false;
            }
            state_ = state;
        }
    }
};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ProjectionTransferService proj_trans_service_;
    ReconstructionService reconstruction_service_;

  public:

    explicit RpcServer(int port);

    void start();

    void updateState(rpc::ServerState_State state);
};


} // recastx::gui::test

#endif //GUI_TEST_RPCSERVER_HPP
