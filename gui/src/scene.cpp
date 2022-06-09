#include <memory>
#include <iostream>

#include <imgui.h>

#include <tomop/tomop.hpp>

#include "graphics/scene_object_2d.hpp"
#include "graphics/scene_object_3d.hpp"
#include "scene.hpp"


namespace gui {

// class Scene

Scene::Scene(const std::string& name, int dimension)
    : name_(name), dimension_(dimension) {
    if (dimension_ == 2) {
        object_ = std::make_unique<SceneObject2d>();
    } else if (dimension_ == 3) {
        object_ = std::make_unique<SceneObject3d>();
    } else {
        throw;
    }
}

Scene::~Scene() = default;

void Scene::render(glm::mat4 window_matrix) { object_->draw(window_matrix); }

// class SceneList

SceneList::SceneList() {
    addScene("TOMCAT live 3D preview", 3);
};

SceneList::~SceneList() = default;

// TODO make thread safe
void SceneList::addScene(const std::string& name, int dimension) {
    auto scene = std::make_unique<Scene>(name, dimension);
    scene->object().add_listener(this);
    scenes_[name] = std::move(scene);
    if (active_scene_ == nullptr) activate(name);
}

void SceneList::describe() {
    ImGui::SetNextWindowSizeConstraints(ImVec2(280, 500), ImVec2(FLT_MAX, FLT_MAX)); // Width > 100, Height > 100
    ImGui::Begin("Scene controls");
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f); // 2/3 of the space for widget and 1/3 for labels

    active_scene_->object().describe();

    ImGui::End();
}

void SceneList::activate(const std::string& name) {
  if (scenes_.find(name) == scenes_.end())
      throw std::runtime_error("Scene " + name + " does not exist!");
  active_scene_ = scenes_[name].get();
}

void SceneList::render(glm::mat4 window_matrix) {
  active_scene_->render(window_matrix);
}

void SceneList::tick(float dt) {
  active_scene_->object().tick(dt);
}

bool SceneList::handle_mouse_button(int button, bool down) {
  return active_scene_->object().handle_mouse_button(button, down);
}

bool SceneList::handle_scroll(double offset) {
  return active_scene_->object().handle_scroll(offset);
}

bool SceneList::handle_mouse_moved(float x, float y) {
  return active_scene_->object().handle_mouse_moved(x, y);
}

bool SceneList::handle_key(int key, bool down, int mods) {
  return active_scene_->object().handle_key(key, down, mods);
}

} // namespace gui
