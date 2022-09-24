#include <spdlog/spdlog.h>

#include <imgui.h>

#include "graphics/scene.hpp"
#include "graphics/camera.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

Scene::Scene(Client* client) : client_(client) {};

Scene::~Scene() = default;

void Scene::onWindowSizeChanged(int width, int height) {
    pos_ = {Style::IMGUI_WINDOW_MARGIN, Style::IMGUI_ICON_HEIGHT + Style::IMGUI_WINDOW_SPACING};
    size_ = {Style::IMGUI_ICON_WIDTH, static_cast<float>(height) - pos_[1] - Style::IMGUI_WINDOW_MARGIN};

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

void Scene::tick(double time_elapsed) {
    auto& packets = Client::packets();
    while (!packets.empty()) {
        auto data = std::move(packets.front());
        packets.pop();

        bool valid = false;
        for (auto& item : data_items_) {
            if (item->consume(data)) valid = true;
        }
        if (!valid) {
            spdlog::warn("Unknown package descriptor: 0x{0:x}",
                         std::underlying_type<PacketDesc>::type(data.first));
        }
    }
}

}  // tomcat::gui
