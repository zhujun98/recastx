#include <spdlog/spdlog.h>

#include <imgui.h>

#include "graphics/scene.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

Scene::Scene() = default;

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

void Scene::init() {
    for (auto &comp : items_) {
        comp->init();
    }
}

void Scene::addItem(GraphicsItem* item) {
    items_.push_back(item);
    auto data_item = dynamic_cast<GraphicsDataItem*>(item);
    if (data_item != nullptr) {
        data_items_.push_back(data_item);
    }
}

bool Scene::handleMouseButton(int button, int action) {
    for (auto& item : items_) {
        if (item->handleMouseButton(button, action)) return true;
    }

    if (!fixed_camera_ && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(float offset) {
    for (auto& item : items_) {
        if (item->handleScroll(offset)) return true;
    }

    if (!fixed_camera_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(float x, float y) {
    for (auto& item : items_) {
        if (item->handleMouseMoved(x, y)) return true;
    }

    if (!fixed_camera_ && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    for (auto& item : items_) {
        if (item->handleKey(key, action, mods)) return true;
    }

    if (!fixed_camera_ && camera_->handleKey(key, action, mods)) return true;
    return false;
}

void Scene::tick(double /*time_elapsed*/) {
    auto& packets = DataClient::packets();
    while (!packets.empty()) {
        auto data = std::move(packets.front());
        packets.pop();

        bool consumed = false;
        for (auto& item : data_items_) {
            if (item->consume(data)) {
                consumed = true;
                break;
            };
        }

        if (!consumed) {
            spdlog::warn("ReconDataPacket ignored!");
        }
    }
}

}  // tomcat::gui
