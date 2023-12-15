/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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
    : vp_(new Viewport()), client_(client) {}

Scene::~Scene() = default;

void Scene::onWindowSizeChanged(int width, int height) {
    size_ = {
        Style::LEFT_PANEL_WIDTH * (float)width,
        (1.f - Style::ICON_HEIGHT - 3.f * Style::MARGIN) * (float)height
    };
    pos_ = {
        Style::MARGIN * (float)width,
        (Style::ICON_HEIGHT + 2.f * Style::MARGIN) * (float)height
    };

    for (auto& item : items_) {
        item->onWindowSizeChanged(width, height);
    }
}

void Scene::useViewport() {
    vp_->use();
}

const glm::mat4& Scene::projectionMatrix() const { return vp_->projection(); }

const glm::mat4& Scene::viewMatrix() const { return camera_->matrix(); }

float Scene::cameraDistance() const { return camera_->distance(); }

void Scene::onFramebufferSizeChanged(int width, int height) {
    vp_->update(0, 0, width, height);

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

    if (!camera_->isFixed() && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(float offset) {
    for (auto& item : items_) {
        if (item->handleScroll(offset)) return true;
    }

    if (!camera_->isFixed() && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(float x, float y) {
    for (auto& item : items_) {
        if (item->handleMouseMoved(x, y)) return true;
    }

    if (!camera_->isFixed() && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    for (auto& item : items_) {
        if (item->handleKey(key, action, mods)) return true;
    }

    if (!camera_->isFixed() && camera_->handleKey(key, action, mods)) return true;
    return false;
}

bool Scene::consume(RpcClient::DataType&& data) {
    for (auto& item : data_items_) {
        if (item->consume(std::move(data))) {
            return true;
        };
    }
    return false;
}

}  // namespace recastx::gui
