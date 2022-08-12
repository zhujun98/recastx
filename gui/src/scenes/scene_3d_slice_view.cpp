#include <algorithm>

#include "graphics/axes_component.hpp"
#include "graphics/geom_component.hpp"
#include "graphics/partitioning_component.hpp"
#include "graphics/recon_component.hpp"
#include "scenes/scene_3d_slice_view.hpp"
#include "scenes/scene_camera3d.hpp"

namespace tomcat::gui {

Scene3dSliceView::Scene3dSliceView() : Scene() {
    camera_ = std::make_unique<SceneCamera3d>();
    this->add_component(std::make_unique<AxesComponent>(*this));
    this->add_component(std::make_unique<ReconComponent>(*this));
    this->add_component(std::make_unique<GeomComponent>(*this));
    this->add_component(std::make_unique<PartitioningComponent>(*this));
}

Scene3dSliceView::~Scene3dSliceView() = default;

void Scene3dSliceView::draw(glm::mat4 window_matrix) {
    auto world_to_screen = window_matrix * camera_->matrix();
    std::vector<ObjectComponent *> drawables;
    for (auto &component : components_) drawables.push_back(component.second.get());

    std::sort(drawables.begin(), drawables.end(), [](auto lptr, auto rptr) {
      return lptr->priority() > rptr->priority();
    });

    for (auto drawable : drawables) drawable->draw(world_to_screen);
}

} // tomcat::gui
