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

    for (auto& comp : components_) {
        comp->onWindowSizeChanged(width, height);
    }
}

void Scene::renderIm() {
    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoResize);
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    camera_->renderIm();

    for (auto &comp : components_) {
        ImGui::Separator();
        comp->renderIm();
    }

    ImGui::End();
}

void Scene::renderGl() {
    viewports_[0]->use();
    for (auto &comp : components_) {
        comp->renderGl();
    }
}

void Scene::init() {
    for (auto &comp : components_) {
        comp->init();
    }
}

void Scene::addComponent(const std::shared_ptr<GraphicsItem>& component) {
    components_.push_back(component);

    if (component->type() == GraphicsItem::ComponentType::STATIC) {
        static_components_.push_back(std::dynamic_pointer_cast<StaticGraphicsItem>(component));
    }
    if (component->type() == GraphicsItem::ComponentType::DYNAMIC) {
        dynamic_components_.push_back(std::dynamic_pointer_cast<DynamicGraphicsItem>(component));
    }
}

bool Scene::handleMouseButton(int button, int action) {
    for (auto& comp : components_) {
        if (comp->handleMouseButton(button, action)) return true;
    }

    if (camera_ && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(float offset) {
    for (auto& comp : components_) {
        if (comp->handleScroll(offset)) return true;
    }

    if (camera_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(float x, float y) {
    for (auto& comp : components_) {
        if (comp->handleMouseMoved(x, y)) return true;
    }

    if (camera_ && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, int action, int mods) {
    for (auto& comp : components_) {
        if (comp->handleKey(key, action, mods)) return true;
    }

    if (camera_ && camera_->handleKey(key, action, mods)) return true;
    return false;
}

void Scene::tick(double time_elapsed) {
    auto& packets = Client::packets();
    while (!packets.empty()) {
        auto data = std::move(packets.front());
        packets.pop();

        bool valid = false;
        for (auto& comp : dynamic_components_) {
            if (comp->consume(data)) valid = true;
        }
        if (!valid) {
            spdlog::warn("Unknown package descriptor: 0x{0:x}",
                         std::underlying_type<PacketDesc>::type(data.first));
        }
    }
}

}  // tomcat::gui
