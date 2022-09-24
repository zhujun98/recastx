#include <algorithm>

#include <spdlog/spdlog.h>

#include "graphics/nodes/axes_component.hpp"
#include "graphics/nodes/monitor_bottom_component.hpp"
#include "graphics/nodes/monitor_right_component.hpp"
#include "graphics/nodes/recon_component.hpp"
#include "graphics/nodes/scene3d.hpp"
#include "graphics/nodes/camera.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client) : Scene(client) {
    viewports_.emplace_back(std::make_unique<Viewport>());
    camera_ = std::make_unique<Camera>();
    this->addComponent(std::make_shared<AxesComponent>(*this));
    this->addComponent(std::make_shared<MonitorBottomComponent>(*this));
    this->addComponent(std::make_shared<MonitorRightComponent>(*this));
    this->addComponent(std::make_shared<ReconComponent>(*this));
}

Scene3d::~Scene3d() = default;

void Scene3d::onFrameBufferSizeChanged(int width, int height) {
    viewports_[0]->update(0, 0, width, height);
}

} // tomcat::gui
