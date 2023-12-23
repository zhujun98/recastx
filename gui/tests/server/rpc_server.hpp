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

class Application;

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

    Application* app_;

  public:

    explicit ControlService(Application* app);

    grpc::Status StartAcquiring(grpc::ServerContext* context,
                                const google::protobuf::Empty* req,
                                google::protobuf::Empty* rep) override;

    grpc::Status StopAcquiring(grpc::ServerContext* context,
                                const google::protobuf::Empty* req,
                                google::protobuf::Empty* rep) override;

    grpc::Status StartProcessing(grpc::ServerContext* context,
                                const google::protobuf::Empty* req,
                                google::protobuf::Empty* rep) override;

    grpc::Status StopProcessing(grpc::ServerContext* context,
                                const google::protobuf::Empty* req,
                                google::protobuf::Empty* rep) override;

    grpc::Status GetServerState(grpc::ServerContext* context,
                                const google::protobuf::Empty* req,
                                rpc::ServerState* state) override;

    grpc::Status SetScanMode(grpc::ServerContext* context,
                             const rpc::ScanMode* mode,
                             google::protobuf::Empty* ack) override;
};

class ImageprocService final : public rpc::Imageproc::Service {

    Application* app_;

public:

    explicit ImageprocService(Application* app);

    grpc::Status SetDownsampling(grpc::ServerContext* context,
                                 const rpc::DownsamplingParams* params,
                                 google::protobuf::Empty* ack) override;

    grpc::Status SetRampFilter(grpc::ServerContext* contest,
                               const rpc::RampFilterParams* params,
                               google::protobuf::Empty* ack) override;
};

class ProjectionTransferService final : public rpc::ProjectionTransfer::Service {

    Application* app_;

    static constexpr uint32_t x_ = 1024;
    static constexpr uint32_t y_ = 1024;

    uint32_t proj_id_;

    void setProjectionData(rpc::ProjectionData* data);

  public:

    explicit ProjectionTransferService(Application* app);

    grpc::Status SetProjection(grpc::ServerContext* context,
                               const rpc::Projection* request,
                               google::protobuf::Empty* ack) override;

    grpc::Status GetProjectionData(grpc::ServerContext* context,
                                   const google::protobuf::Empty*,
                                   grpc::ServerWriter<rpc::ProjectionData>* writer) override;
};

class ReconstructionService final : public rpc::Reconstruction::Service {

    Application* app_;

    static constexpr uint32_t slice_x_ = 1024;
    static constexpr uint32_t slice_y_ = 512;

    static constexpr uint32_t volume_x_ = 128;
    static constexpr uint32_t volume_y_ = 128;
    static constexpr uint32_t volume_z_ = 128;

    std::thread thread_;

    uint64_t timestamp_ {0};

    std::array<uint64_t, MAX_NUM_SLICES> timestamps_;

    void setSliceData(rpc::ReconData* data, int id);

    void setVolumeShardData(rpc::ReconData* data, uint32_t shard_id);

  public:

    explicit ReconstructionService(Application* app);

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const rpc::Slice* slice,
                          google::protobuf::Empty* ack) override;

    grpc::Status SetVolume(grpc::ServerContext* context,
                           const rpc::Volume* volume,
                           google::protobuf::Empty* ack) override;

    grpc::Status GetReconData(grpc::ServerContext* context,
                              const google::protobuf::Empty*,
                              grpc::ServerWriter<rpc::ReconData>* writer) override;

};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ProjectionTransferService proj_trans_service_;
    ReconstructionService reconstruction_service_;

  public:

    RpcServer(int port, Application* app);

    void start();
};


} // recastx::gui::test

#endif //GUI_TEST_RPCSERVER_HPP
