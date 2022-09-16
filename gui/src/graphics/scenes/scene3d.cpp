#include <algorithm>

#include "graphics/scenes/axes_component.hpp"
#include "graphics/scenes/recon_component.hpp"
#include "graphics/scenes/scene3d.hpp"
#include "graphics/scenes/scene_camera3d.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client) : Scene(client) {
    camera_ = std::make_unique<SceneCamera3d>();
    this->addComponent(std::make_unique<AxesComponent>(*this));
    this->addComponent(std::make_unique<ReconComponent>(*this));
}

Scene3d::~Scene3d() = default;

void Scene3d::init() {
    for (auto &component : components_) {
        component.second->init();
    }
}

void Scene3d::describe() {
    Scene::describe();

    for (auto &component : components_) {
        component.second->describe();
    }
}

void Scene3d::render(const glm::mat4& window_matrix) {
    auto matrix = window_matrix * camera_->matrix();
    for (auto &component : components_) {
        component.second->render(matrix);
    }
}

} // tomcat::gui
