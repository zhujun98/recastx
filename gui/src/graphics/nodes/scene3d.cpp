#include <algorithm>

#include <spdlog/spdlog.h>

#include "graphics/nodes/axes_component.hpp"
#include "graphics/nodes/statusbar_component.hpp"
#include "graphics/nodes/recon_component.hpp"
#include "graphics/nodes/scene3d.hpp"
#include "graphics/nodes/scene_camera3d.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client) : Scene(client) {
    camera_ = std::make_unique<SceneCamera3d>();
    this->addComponent(std::make_shared<AxesComponent>(*this));
    this->addComponent(std::make_shared<ReconComponent>(*this));
    this->addComponent(std::make_shared<StatusbarComponent>(*this));
}

Scene3d::~Scene3d() = default;

} // tomcat::gui
