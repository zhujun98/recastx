#include <algorithm>

#include <glm/glm.hpp>

#include "graphics/scene_camera3d.hpp"
#include "graphics/scene_object3d.hpp"
#include "graphics/axes_component.hpp"
#include "graphics/control_component.hpp"
#include "graphics/recon_component.hpp"
#include "graphics/geom_component.hpp"
#include "graphics/partitioning_component.hpp"

namespace gui {

SceneObject3d::SceneObject3d() : SceneObject() {
    camera_ = std::make_unique<SceneCamera3d>();
    this->add_component(std::make_unique<AxesComponent>(*this));
    this->add_component(std::make_unique<ControlComponent>(*this));
    this->add_component(std::make_unique<ReconComponent>(*this));
    this->add_component(std::make_unique<GeomComponent>(*this));
    this->add_component(std::make_unique<PartitioningComponent>(*this));
}

SceneObject3d::~SceneObject3d() = default;

void SceneObject3d::draw(glm::mat4 window_matrix) {
    auto world_to_screen = window_matrix * camera_->matrix();
    std::vector<ObjectComponent *> drawables;
    for (auto &component : components_) drawables.push_back(component.second.get());

    std::sort(drawables.begin(), drawables.end(), [](auto lptr, auto rptr) {
      return lptr->priority() > rptr->priority();
    });

    for (auto drawable : drawables) drawable->draw(world_to_screen);
}

} // namespace gui
