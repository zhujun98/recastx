#ifndef GUI_TEST_APPLICATION_HPP
#define GUI_TEST_APPLICATION_HPP

#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>

#include "control.grpc.pb.h"

namespace recastx::gui::test {

class Application {

    rpc::ServerState_State server_state_ = rpc::ServerState_State_UNKNOWN;

    bool on_demand_ = false;

    bool sino_uploaded_ = false;

    void init() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

public:

    Application() : server_state_(rpc::ServerState_State_READY) {}

    void startAcquiring() {
        assert(server_state_ != rpc::ServerState_State_UNKNOWN);

        init();
        sino_uploaded_ = false;
        server_state_ = rpc::ServerState_State_ACQUIRING;
        spdlog::info("Start acquiring data");
    }

    void stopAcquiring() {
        on_demand_ = false;
        server_state_ = rpc::ServerState_State_READY;
        spdlog::info("Stop acquiring data");
    }

    void startProcessing() {
        assert(server_state_ != rpc::ServerState_State_UNKNOWN);

        init();
        server_state_ = rpc::ServerState_State_PROCESSING;
        spdlog::info("Start acquiring & processing data");
    }

    void stopProcessing() {
        on_demand_ = false;
        server_state_ = rpc::ServerState_State_READY;
        spdlog::info("Stop acquiring & processing data");
    }

    [[nodiscard]] rpc::ServerState_State serverState() const { return server_state_; }

    void setOnDemand(bool state) { on_demand_ = state; }
    [[nodiscard]] bool onDemand() const { return on_demand_; }

    void setSinoUploaded(bool state) { sino_uploaded_ = true; }
    [[nodiscard]] bool sinoUploaded() const { return sino_uploaded_; }
};

} // recastx::gui::test

#endif //GUI_TEST_APPLICATION_HPP
