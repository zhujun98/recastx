#include <imgui.h>
#include <GL/gl3w.h>

#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/scene_camera.hpp"

namespace gui {

Scene::Scene() = default;

Scene::~Scene() {
    glDeleteVertexArrays(1, &vao_handle_);
    glDeleteBuffers(1, &vbo_handle_);
}

void Scene::tick(float time_elapsed) {
    camera_->tick(time_elapsed);
    for (auto& id_and_comp : components_) {
        id_and_comp.second->tick(time_elapsed);
    }
}

void Scene::describe() {
    camera_->describe();

    for (auto& id_and_comp : components_) {
        if (ImGui::CollapsingHeader(id_and_comp.first.c_str())) {
            id_and_comp.second->describe();
        }
    }
}

bool Scene::handleMouseButton(int button, bool down) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleMouseButton(button, down)) return true;
    }

    if (camera_ && camera_->handleMouseButton(button, down)) return true;
    return false;
}

bool Scene::handleScroll(double offset) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleScroll(offset)) return true;
    }

    if (camera_ && camera_->handleScroll(offset)) return true;
    return false;
}

bool Scene::handleMouseMoved(double x, double y) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleMouseMoved(x, y)) return true;
    }

    if (camera_ && camera_->handleMouseMoved(x, y)) return true;
    return false;
}

bool Scene::handleKey(int key, bool down, int mods) {
    for (auto& id_and_comp : components_) {
        if (id_and_comp.second->handleKey(key, down, mods)) return true;
    }

    if (camera_ && camera_->handleKey(key, down, mods)) return true;
    return false;
}

SceneCamera& Scene::camera() { return *camera_; }

}  // namespace gui
