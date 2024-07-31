/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RPCCLIENT_H
#define GUI_RPCCLIENT_H

#include <atomic>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "control.grpc.pb.h"
#include "imageproc.grpc.pb.h"
#include "projection.grpc.pb.h"
#include "reconstruction.grpc.pb.h"

#include "logger.hpp"
#include "common/config.hpp"
#include "common/queue.hpp"

namespace recastx::gui {

using namespace std::string_literals;

class RpcClient {

  public:

    enum class State {
        OK = 0,
        ERROR = 1
    };

    using DataType = std::variant<rpc::ReconData, rpc::ProjectionData>;

    static constexpr int min_timeout = 1;
    static constexpr int max_timeout = 100;

    static std::string serverStateToString(rpc::ServerState_State state) {
        if (state == rpc::ServerState_State_UNKNOWN) return "UNKNOWN";
        if (state == rpc::ServerState_State_INITIALISING) return "INITIALIZING";
        if (state == rpc::ServerState_State_READY) return "READY";
        if (state == rpc::ServerState_State_ACQUIRING) return "ACQUIRING";
        if (state == rpc::ServerState_State_PROCESSING) return "PROCESSING";
        if (state == rpc::ServerState_State_STOPPING) return "STOPPING";
        throw std::runtime_error(fmt::format("Unknown server state: {}", state));
    }

  private:

    std::shared_ptr<grpc::Channel> channel_;

    std::unique_ptr<rpc::Control::Stub> control_stub_;
    std::unique_ptr<rpc::Imageproc::Stub> imageproc_stub_;
    std::unique_ptr<rpc::ProjectionTransfer::Stub> proj_trans_stub_;
    std::unique_ptr<rpc::Reconstruction::Stub> recon_stub_;

    bool streaming_ = false;

    std::thread thread_recon_;

    std::atomic<bool> streaming_proj_ = false;
    std::thread thread_proj_;

    ThreadSafeQueue<DataType> packets_;

    void updateTimeout(int& timeout, const grpc::Status& status) {
        if (checkStatus(status, false) != State::OK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
            timeout = std::min(2 * timeout, max_timeout);
        } else {
            timeout = min_timeout;
        }
    }

    void startReadingProjectionStream();

    void startReadingReconStream();

    [[nodiscard]] State checkStatus(const grpc::Status& status, bool warn_on_unavailable_server = true) const;

  public:

    ThreadSafeQueue<DataType>& packets();

    explicit RpcClient(const std::string& address);

    ~RpcClient();

    State startAcquiring();

    State stopAcquiring();

    State startProcessing();

    State stopProcessing();

    std::optional<rpc::ServerState_State> getServerState();

    State setScanMode(rpc::ScanMode_Mode mode, uint32_t update_interval);

    State setDownsampling(uint32_t col, uint32_t row);

    State setCorrection(int32_t offset, bool minus_log);

    State setRampFilter(const std::string& filter_name);

    State setProjectionGeometry(uint32_t beam_shape,
                               uint32_t col_count, uint32_t row_count, float pixel_width, float pixel_height,
                               float src2origin, float origin2det,
                               uint32_t angle_count, int angle_range);

    State setProjection(uint32_t id);

    State setReconGeometry(uint32_t slice_size, uint32_t volume_size,
                          std::array<int32_t, 2> x, std::array<int32_t, 2> y, std::array<int32_t, 2> z);

    State setSlice(uint64_t timestamp, const Orientation& orientation);

    State setVolume(bool required);

    std::optional<rpc::ServerState_State> shakeHand();

    void startStreaming();

    void toggleProjectionStream(bool state);
};

#define CHECK_CLIENT_STATE(x) { RpcClient::State ret = x;\
    if (ret != RpcClient::State::OK) return ret; }

}  // namespace recastx::gui

#endif //GUI_RPCCLIENT_H