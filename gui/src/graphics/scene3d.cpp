/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>

#include <glm/glm.hpp>

#include "graphics/scene3d.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/items/axiscube_item.hpp"
#include "graphics/items/axes_item.hpp"
#include "graphics/items/icon_item.hpp"
#include "graphics/items/preproc_item.hpp"
#include "graphics/items/projection_item.hpp"
#include "graphics/items/statusbar_item.hpp"
#include "graphics/items/logging_item.hpp"
#include "graphics/items/recon_item.hpp"
#include "logger.hpp"

namespace recastx::gui {

Scene3d::Scene3d(RpcClient* client)
        : Scene(client),
          axes_item_(new AxesItem(*this)),
          icon_item_(new IconItem(*this)),
          preproc_item_(new PreprocItem(*this)),
          projection_item_(new ProjectionItem(*this)),
          recon_item_(new ReconItem(*this)),
          statusbar_item_(new StatusbarItem(*this)),
          logging_item_(new LoggingItem(*this)),
          axiscube_item_(new AxiscubeItem(*this)),
          server_state_(rpc::ServerState_State::ServerState_State_READY),
          scan_mode_(rpc::ScanMode_Mode_CONTINUOUS),
          scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL) {
    camera_ = std::make_unique<Camera>();

    scene_status_["tomoUpdateFrameRate"] = 0.;

    client_->start();
}

Scene3d::~Scene3d() = default;

void Scene3d::onStateChanged(rpc::ServerState_State state) {
    if (state == rpc::ServerState_State_PROCESSING || state == rpc::ServerState_State_ACQUIRING) {
        if (updateServerParams()) return;
        for (auto item : items_) {
            if (item->updateServerParams()) return;
        }
    }
    if (client_->setServerState(state)) return;

    server_state_ = state;
    for (auto item : items_) item->setState(state);

    if (state == rpc::ServerState_State::ServerState_State_PROCESSING) {
        log::info("Start acquiring & processing data");
    } else if (state == rpc::ServerState_State::ServerState_State_ACQUIRING) {
        log::info("Start acquiring data");
    } else /* (state == ServerState_State::ServerState_State_READY) */ {
        log::info("Stop acquiring & processing data");
    }
}

bool Scene3d::updateServerParams() {
    return setScanMode();
}

bool Scene3d::setScanMode() {
    return client_->setScanMode(scan_mode_, scan_update_interval_);
}

void Scene3d::render() {
    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoResize);
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.2f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.2f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.2f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ == rpc::ServerState_State::ServerState_State_ACQUIRING ||
                         server_state_ == rpc::ServerState_State::ServerState_State_PROCESSING);
    if (ImGui::Button("Acquire")) onStateChanged(rpc::ServerState_State::ServerState_State_ACQUIRING);
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ == rpc::ServerState_State::ServerState_State_PROCESSING ||
                         server_state_ == rpc::ServerState_State::ServerState_State_ACQUIRING);
    if (ImGui::Button("Process")) onStateChanged(rpc::ServerState_State::ServerState_State_PROCESSING);
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State::ServerState_State_PROCESSING &&
                         server_state_ != rpc::ServerState_State::ServerState_State_ACQUIRING);
    if (ImGui::Button("Stop")) onStateChanged(rpc::ServerState_State::ServerState_State_READY);
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    // ---
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "SCAN MODE");

    ImGui::BeginDisabled(server_state_ == rpc::ServerState_State::ServerState_State_PROCESSING);

    static int scan_mode = static_cast<int>(scan_mode_);
    ImGui::RadioButton("Continuous", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_CONTINUOUS));
    ImGui::SameLine();
    ImGui::RadioButton("Discrete", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_DISCRETE));
    scan_mode_ = rpc::ScanMode_Mode(scan_mode);

    ImGui::Text("Update interval");
    ImGui::SameLine();

    ImGui::BeginDisabled(scan_mode_ != rpc::ScanMode_Mode_CONTINUOUS);
    if (ImGui::ArrowButton("##continuous_interval_left", ImGuiDir_Left)) {
        assert(scan_update_interval_ >= K_SCAN_UPDATE_INTERVAL_STEP_SIZE);
        scan_update_interval_ -= K_SCAN_UPDATE_INTERVAL_STEP_SIZE;
        if (scan_update_interval_ < K_MIN_SCAN_UPDATE_INTERVAL) {
            scan_update_interval_ = K_MIN_SCAN_UPDATE_INTERVAL;
        }
    }
    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##continuous_interval_right", ImGuiDir_Right)) {
        scan_update_interval_ += K_SCAN_UPDATE_INTERVAL_STEP_SIZE;
        if (scan_update_interval_ > K_MAX_SCAN_UPDATE_INTERVAL) {
            scan_update_interval_ = K_MAX_SCAN_UPDATE_INTERVAL;
        }
    }
    ImGui::SameLine();
    ImGui::Text("%d", scan_update_interval_);
    ImGui::EndDisabled();

    ImGui::EndDisabled();

    ImGui::Separator();
    projection_item_->renderIm();

    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "CAMERA");
    ImGui::Checkbox("Fix camera", &fixed_camera_);
    if (ImGui::Button("X-Y")) {
        camera_->setTopView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        camera_->setFrontView();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        camera_->setSideView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective")) {
        camera_->setPerspectiveView();
    }
    axes_item_->renderIm();

    ImGui::Separator();
    preproc_item_->renderIm();
    ImGui::Separator();
    recon_item_->renderIm();
    ImGui::Separator();
    statusbar_item_->renderIm();
    ImGui::Separator();
    logging_item_->renderIm();

    ImGui::End();

    for (auto& item: gl_items_) {
        item->renderGl();
    }
}

} // namespace recastx::gui
