#ifndef RECON_ZMQSERVER_H
#define RECON_ZMQSERVER_H

#include <array>
#include <mutex>
#include <string>
#include <thread>

#include <zmq.hpp>

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::recon {

class Application;

class DataServer {

    zmq::context_t context_;
    zmq::socket_t socket_;

    std::thread thread_;

    std::mutex send_mtx_;

    Application* app_;

public:

    DataServer(int port, Application* app);

    ~DataServer();

    template<typename T>
    void send(const T& data);

    void start();
};

template<typename T>
void DataServer::send(const T& packet) {
    std::string encoded;
    packet.SerializeToString(&encoded);

    zmq::message_t msg;
    socket_.recv(msg, zmq::recv_flags::none);
    auto request = std::string(static_cast<char*>(msg.data()), msg.size());
    if (request == "GUIReady") {
        socket_.send(zmq::buffer(std::move(encoded)), zmq::send_flags::none);
    } else {
        spdlog::warn("Unknown request received: {}", request);
    }
}


class ControlService final : public Control::Service {

    Application* app_;

  public:

    ControlService(Application* app);

    grpc::Status SetServerState(grpc::ServerContext* context, 
                                const ServerState* state,
                                google::protobuf::Empty* ack);

};

class ImageprocService final : public Imageproc::Service {

    Application* app_;

  public:

    ImageprocService(Application* app);

    grpc::Status SetDownsamplingParams(grpc::ServerContext* context, 
                                       const DownsamplingParams* params,
                                       google::protobuf::Empty* ack);
};

class ReconstructionService final : public Reconstruction::Service {

    Application* app_;

  public:

    ReconstructionService(Application* app);

    grpc::Status SetSlice(grpc::ServerContext* context, 
                          const Slice* slice,
                          google::protobuf::Empty* ack);
};

class RpcServer {

    std::unique_ptr<grpc::Server> server_;
    std::string address_;

    ControlService control_service_;
    ImageprocService imageproc_service_;
    ReconstructionService reconstruction_service_;

    std::thread thread_;

public:

    RpcServer(int port, Application* app);

    void start();
};


} // namespace recastx::recon

#endif // RECON_ZMQSERVER_H