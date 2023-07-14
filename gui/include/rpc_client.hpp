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

#include <zmq.hpp>

#include "logger.hpp"
#include "common/config.hpp"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

namespace recastx::gui {

using namespace std::string_literals;

class RpcClient {

    std::shared_ptr<grpc::Channel> channel_;

    std::unique_ptr<Control::Stub> control_stub_;
    std::unique_ptr<Imageproc::Stub> imageproc_stub_;
    std::unique_ptr<Reconstruction::Stub> reconstruction_stub_;

    std::thread thread_;
    bool streaming_ = false;

    inline static std::queue<ReconData> packets_;

    bool checkStatus(const grpc::Status& status, bool warn_on_fail = true) const;

  public:

    static std::queue<ReconData>& packets();

    RpcClient(const std::string& hostname, int port);

    ~RpcClient();

    bool setServerState(ServerState_State state);

    bool setScanMode(ScanMode_Mode mode, uint32_t update_interval);

    bool setDownsampling(uint32_t col, uint32_t row);

    bool setRampFilter(const std::string& filter_name);

    bool setSlice(uint64_t timestamp, const Orientation& orientation);

    void startReconDataStream();

    void stopReconDataStream();
};

}  // namespace recastx::gui

#endif //GUI_ZMQCLIENT_H