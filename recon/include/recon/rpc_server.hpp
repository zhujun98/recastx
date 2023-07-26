/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

class ControlService final : public Control::Service {

    Application* app_;

  public:

    explicit ControlService(Application* app);

    grpc::Status SetServerState(grpc::ServerContext* context, 
                                const ServerState* state,
                                google::protobuf::Empty* ack) override;

    grpc::Status SetScanMode(grpc::ServerContext* context,
                             const ScanMode* mode,
                             google::protobuf::Empty* ack) override;

};

class ImageprocService final : public Imageproc::Service {

    Application* app_;

  public:

    explicit ImageprocService(Application* app);

    grpc::Status SetDownsampling(grpc::ServerContext* context, 
                                 const DownsamplingParams* params,
                                 google::protobuf::Empty* ack) override;

    grpc::Status SetRampFilter(grpc::ServerContext* contest,
                               const RampFilterParams* params,
                               google::protobuf::Empty* ack) override;
};

class ProjectionService final : public rpc::Projection::Service {

    Application* app_;

  public:

    explicit ProjectionService(Application* app);

    grpc::Status GetProjectionData(grpc::ServerContext* context,
                                   const google::protobuf::Empty*,
                                   grpc::ServerWriter<rpc::ProjectionData>* writer) override;

};

class ReconstructionService final : public Reconstruction::Service {

    std::thread thread_;

    Application* app_;

  public:

    explicit ReconstructionService(Application* app);

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const Slice* slice,
                          google::protobuf::Empty* ack) override;

    grpc::Status GetReconData(grpc::ServerContext* context,
                              const google::protobuf::Empty*,
                              grpc::ServerWriter<ReconData>* writer) override;
};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ProjectionService projection_service_;
    ReconstructionService reconstruction_service_;

    std::thread thread_;

public:

    RpcServer(int port, Application* app);

    void start();
};


} // namespace recastx::recon

#endif // RECON_RPCSERVER_H