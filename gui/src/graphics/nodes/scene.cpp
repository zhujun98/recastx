#include <imgui.h>

#include "graphics/nodes/scene.hpp"
#include "graphics/nodes/scene_camera.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

Scene::Scene(Client* client) : client_(client) {};

Scene::~Scene() {
    glDeleteVertexArrays(1, &vao_handle_);
    glDeleteBuffers(1, &vbo_handle_);
}

void Scene::describe() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(280, 500), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Image tool (3D)");
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    camera_->describe();

    for (auto &comp : components_) {
        comp->describe();
    }
}

void Scene::render(const glm::mat4& window_matrix) {
    auto matrix = window_matrix * camera_->matrix();
    for (auto &comp : components_) {
        comp->render(matrix);
    }
}

void Scene::init() {
    for (auto &comp : components_) {
        comp->init();
    }
}

void Scene::addComponent(const std::shared_ptr<SceneComponent>& component) {
    components_.insert(component);

    if (component->type() == SceneComponent::ComponentType::STATIC) {
        static_components_.insert(std::dynamic_pointer_cast<StaticSceneComponent>(component));
    }
    if (component->type() == SceneComponent::ComponentType::DYNAMIC) {
        dynamic_components_.insert(std::dynamic_pointer_cast<DynamicSceneComponent>(component));
    }
}

bool Scene::handleMouseButton(int button, int action) {
    for (auto& comp : components_) {
        if (comp->handleMouseButton(button, action)) return true;
    }

    if (camera_ && camera_->handleMouseButton(button, action)) return true;
    return false;
}

bool Scene::handleScroll(double offset) {
    for (auto& comp : components_) {
        if (comp->handleScroll(offset)) return true;
    }

    if (camera_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(double x, double y) {
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

SceneCamera& Scene::camera() { return *camera_; }

void Scene::tick(double time_elapsed) {
    camera_->tick(time_elapsed);

    auto& packets = Client::packets();
    while (!packets.empty()) {
        auto data = std::move(packets.front());
        packets.pop();

        for (auto& comp : dynamic_components_) {
            comp->consume(data);
        }
    }
}

}  // tomcat::gui
