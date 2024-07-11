/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/scene.hpp"
#include "graphics/camera.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"
#include "graphics/viewport.hpp"
#include "graphics/items/axis_item.hpp"
#include "graphics/items/axiscube_item.hpp"
#include "graphics/items/icon_item.hpp"
#include "graphics/items/geometry_item.hpp"
#include "graphics/items/light_item.hpp"
#include "graphics/items/logging_item.hpp"
#include "graphics/items/material_item.hpp"
#include "graphics/items/preproc_item.hpp"
#include "graphics/items/projection_item.hpp"
#include "graphics/items/recon_item.hpp"
#include "graphics/items/statusbar_item.hpp"

namespace recastx::gui {

Scene::Scene(RpcClient* client)
    : camera_(new Camera()),
      client_(client),
      scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL),
      axis_item_(new AxisItem(*this)),
      axiscube_item_(new AxisCubeItem(*this)),
      icon_item_(new IconItem(*this)),
      geometry_item_(new GeometryItem(*this)),
      light_item_(new LightItem(*this)),
      logging_item_(new LoggingItem(*this)),
      material_item_(new MaterialItem(*this)),
      preproc_item_(new PreprocItem(*this)),
      projection_item_(new ProjectionItem(*this)),
      recon_item_(new ReconItem(*this)),
      statusbar_item_(new StatusbarItem(*this)) {
    axis_item_->linkViewport(recon_item_->viewport());
    light_item_->linkViewport(recon_item_->viewport());
}

Scene::~Scene() = default;

void Scene::onWindowSizeChanged(int width, int height) {
    updateLayout(width, height);

    for (auto& item : items_) {
        item->onWindowSizeChanged(width, height);
    }
}

const Light& Scene::light() const {
    return light_item_->light();
}

const Material& Scene::material() const {
    return material_item_->material();
}

const glm::mat4& Scene::cameraMatrix() const { return camera_->matrix(); }

float Scene::cameraDistance() const { return camera_->distance(); }

glm::vec3 Scene::cameraDir() const { return camera_->viewDir(); }

glm::vec3 Scene::cameraPosition() const { return camera_->pos(); }

void Scene::onFramebufferSizeChanged(int width, int height) {
    layout_.sw = (double)width / layout_.w;
    layout_.sh = (double)height / layout_.h;

    for (auto& item : gl_items_) {
        item->onFramebufferSizeChanged(width, height);
    }
}

void Scene::render() {
    for (auto& item: gl_items_) {
        item->preRenderGl();
        item->renderGl();
    }

    const auto& l = layout_;

    ImGui::SetNextWindowPos(ImVec2(l.mw, l.mh * 2 + l.th));
    ImGui::SetNextWindowSize(ImVec2(l.lw, l.h - 3 * l.mh - l.th));

    ImGui::Begin("Control Panel Left", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);
    renderMainControl();
    ImGui::Separator();
    renderScanModeControl();
    ImGui::Separator();
    renderCameraControl();
    projection_item_->renderIm();
    geometry_item_->renderIm();
    preproc_item_->renderIm();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(l.w - l.mw - l.rw, l.mh * 2 + l.th));
    ImGui::SetNextWindowSize(ImVec2(l.rw, l.h - 3 * l.mh - l.th));

    ImGui::Begin("Control Panel Right", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar);
    renderMenubarRight();
    recon_item_->renderIm();
    light_item_->renderIm();
    material_item_->renderIm();

    ImGui::End();

    statusbar_item_->renderIm();

    logging_item_->renderIm();
}

void Scene::addItem(GraphicsItem* item) {
    items_.push_back(item);

    auto gl_item = dynamic_cast<GraphicsGLItem*>(item);
    if (gl_item != nullptr) {
        gl_items_.push_back(gl_item);
    }

    auto data_item = dynamic_cast<GraphicsDataItem*>(item);
    if (data_item != nullptr) {
        data_items_.push_back(data_item);
    }
}

bool Scene::handleMouseButton(int button, int action) {
    for (auto& item : items_) {
        if (item->handleMouseButton(button, action)) return true;
    }

    if (!camera_->fixed() && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(float offset) {
    for (auto& item : items_) {
        if (item->handleScroll(offset)) return true;
    }

    if (!camera_->fixed() && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(float x, float y) {
    for (auto& item : items_) {
        if (item->handleMouseMoved(x, y)) return true;
    }

    if (!camera_->fixed() && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    for (auto& item : items_) {
        if (item->handleKey(key, action, mods)) return true;
    }

    if (!camera_->fixed() && camera_->handleKey(key, action, mods)) return true;
    return false;
}

bool Scene::consume(const RpcClient::DataType& data) {
    for (auto& item : data_items_) {
        if (item->consume(data)) {
            return true;
        };
    }
    return false;
}

void Scene::connectServer() {
    auto server_state = client_->shakeHand();
    if (server_state) {
        auto s = server_state.value();

        log::info("Connected to reconstruction server");
        if (s == rpc::ServerState_State_UNKNOWN) {
            log::error("Reconstruction server in {} state", RpcClient::serverStateToString(s));
        } else {
            log::info("Reconstruction server in {} state", RpcClient::serverStateToString(s));
            if (s != rpc::ServerState_State_READY) {
                log::warn("Client and reconstruction server are not synchronised. "
                          "Manually stop and restart data acquisition and/or processing");
            }
        }

        server_state_ = server_state.value();
    }
}

void Scene::startAcquiring() {
    if (updateServerParams()) {
        server_state_ = rpc::ServerState_State_UNKNOWN;
        return;
    }
    if (!client_->startAcquiring()) {
        server_state_ = rpc::ServerState_State_ACQUIRING;
        log::info("Started acquiring data");
    }
}

void Scene::stopAcquiring() {
    if (!client_->stopAcquiring()) {
        server_state_ = rpc::ServerState_State_READY;
        log::info("Stopped acquiring data");
    } else {
        server_state_ = rpc::ServerState_State_UNKNOWN;
    }
}

void Scene::startProcessing() {
    if (updateServerParams()) {
        server_state_ = rpc::ServerState_State_UNKNOWN;
        return;
    }
    if (!client_->startProcessing()) {
        server_state_ = rpc::ServerState_State_PROCESSING;
        log::info("Started acquiring & processing data");
    }
}

void Scene::stopProcessing() {
    if (!client_->stopProcessing()) {
        server_state_ = rpc::ServerState_State_READY;
        log::info("Stopped acquiring & processing data");
    } else {
        server_state_ = rpc::ServerState_State_UNKNOWN;
    }
}

void Scene::updateLayout(int width, int height) {
    layout_.w = width;
    layout_.h = height;
    layout_.mw = std::min(Style::MARGIN_MAX, int(Style::MARGIN * (float)width));
    layout_.mh = std::min(Style::MARGIN_MAX, int(Style::MARGIN * (float)height));
    layout_.lw = int(Style::LEFT_PANEL_WIDTH * (float)width);
    layout_.rw = int(Style::RIGHT_PANEL_WIDTH * (float)width);
    layout_.th = int(Style::TOP_PANEL_HEIGHT * (float)height);
    layout_.bh = int(Style::BOTTOM_PANEL_HEIGHT * (float)height);
}

bool Scene::setScanMode() {
    return client_->setScanMode(scan_mode_, scan_update_interval_);
}

bool Scene::updateServerParams() {
    if (setScanMode()) return true;
    for (auto item : items_) {
        if (item->updateServerParams()) return true;
    }
    return false;
}


void Scene::renderMenubarRight() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {

            static bool show_status = statusbar_item_->visible();
            if (ImGui::Checkbox("Show status##VIEW", &show_status)) {
                statusbar_item_->setVisible(show_status);
            }

            static bool show_histogram = recon_item_->histogramVisible();
            if (ImGui::Checkbox("Show histogram##VIEW", &show_histogram)) {
                recon_item_->setHistogramVisible(show_histogram);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Scene::renderMainControl() {
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

void Scene::renderScanModeControl() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "SCAN MODE");

    ImGui::BeginDisabled(server_state_ == rpc::ServerState_State_PROCESSING);

    static int scan_mode = static_cast<int>(scan_mode_);
    ImGui::RadioButton("Static", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_STATIC));
    ImGui::SameLine();
    ImGui::RadioButton("Dynamic", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_DYNAMIC));
    ImGui::SameLine();
    ImGui::RadioButton("Continuous", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_CONTINUOUS));
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

void Scene::renderCameraControl() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "CAMERA");

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

}  // namespace recastx::gui
