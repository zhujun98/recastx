/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>

#include <glm/glm.hpp>

#include "graphics/scene3d.hpp"
#include "graphics/items/axis_item.hpp"
#include "graphics/items/axiscube_item.hpp"
#include "graphics/items/icon_item.hpp"
#include "graphics/items/geometry_item.hpp"
#include "graphics/items/preproc_item.hpp"
#include "graphics/items/projection_item.hpp"
#include "graphics/items/statusbar_item.hpp"
#include "graphics/items/logging_item.hpp"
#include "graphics/items/recon_item.hpp"
#include "logger.hpp"

namespace recastx::gui {

Scene3d::Scene3d(RpcClient* client)
        : Scene(client),
          axis_item_(new AxisItem(*this)),
          axiscube_item_(new AxisCubeItem(*this)),
          icon_item_(new IconItem(*this)),
          geometry_item_(new GeometryItem(*this)),
          preproc_item_(new PreprocItem(*this)),
          projection_item_(new ProjectionItem(*this)),
          recon_item_(new ReconItem(*this)),
          statusbar_item_(new StatusbarItem(*this)),
          logging_item_(new LoggingItem(*this)) {
    axis_item_->linkViewport(recon_item_->viewport());
}

Scene3d::~Scene3d() = default;

void Scene3d::render() {
    for (auto& item: gl_items_) {
        item->preRenderGl();
        item->renderGl();
    }

    ImGui::SetNextWindowPos(ImVec2(layout_.mw, layout_.mh * 2 + layout_.th));
    ImGui::SetNextWindowSize(ImVec2(layout_.lw, layout_.h - 2 * layout_.mh - layout_.th));

    ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);

    renderMenubar();

    renderMainControl();
    ImGui::Separator();
    renderScanModeControl();
    ImGui::Separator();
    renderCameraControl();

    projection_item_->renderIm();
    geometry_item_->renderIm();
    preproc_item_->renderIm();
    recon_item_->renderIm();
    statusbar_item_->renderIm();
    logging_item_->renderIm();

    ImGui::End();
}

void Scene3d::renderMenubar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {

            static bool show_histogram = recon_item_->histogramVisible();
            if (ImGui::Checkbox("Show histogram##VIEW", &show_histogram)) {
                recon_item_->setHistogramVisible(show_histogram);
            }

            static bool show_wireframe = recon_item_->wireframeVisible();
            if (ImGui::Checkbox("Show wireframe##VIEW", &show_wireframe)) {
                recon_item_->setWireframeVisible(show_wireframe);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Render")) {
            if (ImGui::BeginMenu("Volume quality")) {
                static int quality = recon_item_->volumeRenderQuality();
                bool cd = ImGui::RadioButton("Low##VOLUME_RENDER_QUALITY", &quality, RenderQuality::LOW);
                ImGui::SameLine();
                cd |= ImGui::RadioButton("Medium##VOLUME_RENDER_QUALITY", &quality, RenderQuality::MEDIUM);
                ImGui::SameLine();
                cd |= ImGui::RadioButton("High##VOLUME_RENDER_QUALITY", &quality, RenderQuality::HIGH);
                ImGui::EndMenu();

                if (cd) recon_item_->setVolumeRenderQuality(RenderQuality(quality));
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Scene3d::renderMainControl() {
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.2f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.2f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.2f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_UNKNOWN);
    if (ImGui::Button("Connect")) connectServer();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_READY);
    if (ImGui::Button("Acquire")) startAcquiring();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.3f, 0.8f, 0.8f));
    ImGui::BeginDisabled(server_state_ != rpc::ServerState_State_READY);
    if (ImGui::Button("Process")) startProcessing();
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
    ImGui::BeginDisabled(!(server_state_ & rpc::ServerState_State_PROCESSING));
    if (ImGui::Button("Stop")) {
        if (server_state_ == rpc::ServerState_State_ACQUIRING) stopAcquiring();
        else if (server_state_ == rpc::ServerState_State_PROCESSING) stopProcessing();
        else log::warn("Stop from unexpected server state: {}", RpcClient::serverStateToString(server_state_));
    };
    ImGui::EndDisabled();
    ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();
}

void Scene3d::renderScanModeControl() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "SCAN MODE");

    ImGui::BeginDisabled(server_state_ == rpc::ServerState_State_PROCESSING);

    static int scan_mode = static_cast<int>(scan_mode_);
    ImGui::RadioButton("Continuous", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_CONTINUOUS));
    ImGui::SameLine();
    ImGui::RadioButton("Discrete", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_DISCRETE));
    scan_mode_ = rpc::ScanMode_Mode(scan_mode);

    ImGui::AlignTextToFramePadding();
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
}

void Scene3d::renderCameraControl() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "CAMERA");

    static bool fixed = false;
    if (ImGui::Checkbox("Fix camera", &fixed)) {
        camera_->setFixed(fixed);
    }

    if (ImGui::Button("Y-Z##CAMERA")) {
        camera_->setFrontView();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z##CAMERA")) {
        camera_->setSideView();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Y##CAMERA")) {
        camera_->setTopView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective##CAMERA")) {
        camera_->setPerspectiveView();
    }

    ImGui::Separator();
}

} // namespace recastx::gui