/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_ZMQCLIENT_H
#define GUI_ZMQCLIENT_H

#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <variant>

#include <zmq.hpp>

#include "logger.hpp"
#include "common/config.hpp"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "projection.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::gui {

using namespace std::string_literals;

class RpcClient {

  public:

    using DataType = std::variant<rpc::ReconData, rpc::ProjectionData>;

    static constexpr int min_timeout = 1;
    static constexpr int max_timeout = 100;

  private:

    std::shared_ptr<grpc::Channel> channel_;

    std::unique_ptr<rpc::Control::Stub> control_stub_;
    std::unique_ptr<rpc::Imageproc::Stub> imageproc_stub_;
    std::unique_ptr<rpc::Projection::Stub> projection_stub_;
    std::unique_ptr<rpc::Reconstruction::Stub> reconstruction_stub_;

    std::thread thread_projection_;
    std::thread thread_recon_;
    bool streaming_ = false;

    inline static std::queue<DataType> packets_;

    inline void updateTimeout(int& timeout, const grpc::Status& status) {
        if (checkStatus(status, false)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
            timeout = std::min(2 * timeout, max_timeout);
        } else {
            timeout = min_timeout;
        }
    }

    void startReadingProjectionStream();

    void startReadingReconStream();

    [[nodiscard]] bool checkStatus(const grpc::Status& status, bool warn_on_fail = true) const;

  public:

    static std::queue<DataType>& packets();

    explicit RpcClient(const std::string& address);

    ~RpcClient();

    bool setServerState(rpc::ServerState_State state);

    bool setScanMode(rpc::ScanMode_Mode mode, uint32_t update_interval);

    bool setDownsampling(uint32_t col, uint32_t row);

    bool setRampFilter(const std::string& filter_name);

    bool setSlice(uint64_t timestamp, const Orientation& orientation);

    void start();
};

}  // namespace recastx::gui

#endif //GUI_ZMQCLIENT_H