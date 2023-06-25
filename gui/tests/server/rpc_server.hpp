#ifndef GUI_TEST_RPCSERVER_HPP
#define GUI_TEST_RPCSERVER_HPP

#include <array>
#include <string>
#include <thread>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::gui::test {


class ControlService final : public Control::Service {

  public:

    grpc::Status SetServerState(grpc::ServerContext* context,
                                const ServerState* state,
                                google::protobuf::Empty* ack) override;

};

class ImageprocService final : public Imageproc::Service {

  public:

    grpc::Status SetDownsamplingParams(grpc::ServerContext* context,
                                       const DownsamplingParams* params,
                                       google::protobuf::Empty* ack) override;
};

class ReconstructionService final : public Reconstruction::Service {

    std::thread thread_;

    uint64_t timestamp_ {0};

    void setSliceData(ReconData* data);

    void setVolumeData(ReconData* data);

  public:

    grpc::Status SetSlice(grpc::ServerContext* context,
                          const Slice* slice,
                          google::protobuf::Empty* ack) override;

    grpc::Status GetReconData(grpc::ServerContext* context,
                              const google::protobuf::Empty*,
                              ReconData* data) override;
};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ReconstructionService reconstruction_service_;

  public:

    explicit RpcServer(int port);

    void start();
};


} // recastx::gui::test

#endif //GUI_TEST_RPCSERVER_HPP
