#include <algorithm>

#include <imgui.h>

#include "graphics/axes_component.hpp"
#include "graphics/geom_component.hpp"
#include "graphics/partitioning_component.hpp"
#include "graphics/recon_component.hpp"
#include "scenes/overview_scene.hpp"
#include "scenes/scene_camera3d.hpp"

namespace tomcat::gui {

OverviewScene::OverviewScene() : Scene() {
    camera_ = std::make_unique<SceneCamera3d>();
    this->add_component(std::make_unique<AxesComponent>(*this));
    this->add_component(std::make_unique<ReconComponent>(*this));
}

OverviewScene::~OverviewScene() = default;

void OverviewScene::describe() {
    Scene::describe();

    get_component("axes").describe();
    get_component("reconstruction").describe();
}

void OverviewScene::draw(glm::mat4 window_matrix) {
    auto world_to_screen = window_matrix * camera_->matrix();
    std::vector<ObjectComponent *> drawables;
    for (auto &component : components_) drawables.push_back(component.second.get());

    std::sort(drawables.begin(), drawables.end(), [](auto lptr, auto rptr) {
      return lptr->priority() > rptr->priority();
    });

    for (auto drawable : drawables) drawable->draw(world_to_screen);
}

} // tomcat::gui
