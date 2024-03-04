/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <spdlog/spdlog.h>

#include <imgui.h>

#include "graphics/scene.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

Scene::Scene(RpcClient* client)
    : camera_(new Camera()),
      client_(client),
      scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL) {}

Scene::~Scene() = default;

void Scene::onWindowSizeChanged(int width, int height) {
    updateLayout(width, height);

    for (auto& item : items_) {
        item->onWindowSizeChanged(width, height);
    }
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

}  // namespace recastx::gui
