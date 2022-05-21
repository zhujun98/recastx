#include <memory>
#include <iostream>

#include <imgui.h>

#include <tomop/tomop.hpp>

#include "graphics/scene_object_2d.hpp"
#include "graphics/scene_object_3d.hpp"
#include "scene.hpp"


namespace gui {

Scene::Scene(const std::string& name, int dimension, int scene_id)
    : name_(name), dimension_(dimension), scene_id_(scene_id) {
    if (dimension_ == 2) {
        object_ = std::make_unique<SceneObject2d>(scene_id_);
    } else if (dimension_ == 3) {
        object_ = std::make_unique<SceneObject3d>(scene_id_);
    } else {
        throw;
    }
}

Scene::~Scene() {
    auto packet = KillScenePacket(scene_id_);
    object_->send(packet);
}

void Scene::render(glm::mat4 window_matrix) { object_->draw(window_matrix); }


SceneList::SceneList() = default;

SceneList::~SceneList() = default;

// TODO make thread safe
int SceneList::add_scene(const std::string& name,
                         int id,
                         bool make_active,
                         int dimension) {
  if (id == -1) id = reserve_id();

  scenes_[id] = std::make_unique<Scene>(name, dimension, id);
  if (make_active) set_active_scene(id);

  scenes_[id]->object().add_listener(this);

  return id;
}

void SceneList::describe() {
  if (active_scene_ == nullptr) return;

  ImGui::SetNextWindowSizeConstraints(ImVec2(280, 500), ImVec2(FLT_MAX, FLT_MAX)); // Width > 100, Height > 100
  ImGui::Begin("Scene controls");
  ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f); // 2/3 of the space for widget and 1/3 for labels

  active_scene_->object().describe();

  ImGui::End();
}

// TODO make thread safe
int SceneList::reserve_id() { return give_away_id_++; }

void SceneList::set_active_scene(int index) {
  active_scene_ = scenes_[index].get();
  active_scene_index_ = index;
}

void SceneList::render(glm::mat4 window_matrix) {
  if (active_scene_) active_scene_->render(window_matrix);
}

void SceneList::tick(float dt) {
  if (active_scene_) active_scene_->object().tick(dt);
}

bool SceneList::handle_mouse_button(int button, bool down) {
  if (active_scene_) return active_scene_->object().handle_mouse_button(button, down);
  return false;
}

bool SceneList::handle_scroll(double offset) {
  if (active_scene_) return active_scene_->object().handle_scroll(offset);
  return false;
}

bool SceneList::handle_mouse_moved(float x, float y) {
  if (active_scene_) return active_scene_->object().handle_mouse_moved(x, y);
  return false;
}

bool SceneList::handle_key(int key, bool down, int mods) {
  if (active_scene_) return active_scene_->object().handle_key(key, down, mods);
  return false;
}

} // namespace gui
