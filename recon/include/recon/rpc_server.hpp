/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECON_RPCSERVER_H
#define RECON_RPCSERVER_H

#include <array>
#include <string>
#include <thread>

#include <zmq.hpp>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "projection.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::recon {

class Application;

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
                             google::protobuf::Empty* rep) override;

};

class ImageprocService final : public rpc::Imageproc::Service {

    Application* app_;

  public:

    explicit ImageprocService(Application* app);

    grpc::Status SetDownsampling(grpc::ServerContext* context, 
                                 const rpc::DownsamplingParams* params,
                                 google::protobuf::Empty* rep) override;

    grpc::Status SetCorrection(grpc::ServerContext* context,
                               const rpc::CorrectionParams* params,
                               google::protobuf::Empty* ack) override;

    grpc::Status SetRampFilter(grpc::ServerContext* contest,
                               const rpc::RampFilterParams* params,
                               google::protobuf::Empty* rep) override;
};

class ProjectionTransferService final : public rpc::ProjectionTransfer::Service {

    Application* app_;

  public:

    explicit ProjectionTransferService(Application* app);

    grpc::Status SetProjectionGeometry(grpc::ServerContext* context,
                                       const rpc::ProjectionGeometry* geometry,
                                       google::protobuf::Empty* ack) override;

    grpc::Status SetProjection(grpc::ServerContext* context,
                               const rpc::Projection* request,
                               google::protobuf::Empty* rep) override;

    grpc::Status GetProjectionData(grpc::ServerContext* context,
                                   const google::protobuf::Empty*,
                                   grpc::ServerWriter<rpc::ProjectionData>* writer) override;

};

class ReconstructionService final : public rpc::Reconstruction::Service {

    std::thread thread_;

    Application* app_;

  public:

    explicit ReconstructionService(Application* app);

    grpc::Status SetReconGeometry(grpc::ServerContext* context,
                                  const rpc::ReconGeometry* geometry,
                                  google::protobuf::Empty* ack) override;

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const rpc::Slice* slice,
                          google::protobuf::Empty* rep) override;

    grpc::Status SetVolume(grpc::ServerContext* context,
                           const rpc::Volume* volume,
                           google::protobuf::Empty* rep) override;

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
    ReconstructionService recon_service_;

    std::thread thread_;

public:

    RpcServer(int port, Application* app);

    void start();
};


} // namespace recastx::recon

#endif // RECON_RPCSERVER_H