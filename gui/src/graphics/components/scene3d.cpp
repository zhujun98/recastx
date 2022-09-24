#include <algorithm>

#include <spdlog/spdlog.h>

#include "graphics/components/axes_component.hpp"
#include "graphics/components/statusbar_component.hpp"
#include "graphics/components/recon_component.hpp"
#include "graphics/components/scene3d.hpp"
#include "graphics/components/camera.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client) : Scene(client) {
    viewports_.emplace_back(std::make_unique<Viewport>());
    camera_ = std::make_unique<Camera>();
    this->addComponent(std::make_shared<AxesComponent>(*this));
    this->addComponent(std::make_shared<StatusbarComponent>(*this));
    this->addComponent(std::make_shared<ReconComponent>(*this));
}

Scene3d::~Scene3d() = default;

void Scene3d::onFrameBufferSizeChanged(int width, int height) {
    viewports_[0]->update(0, 0, width, height);
}

} // tomcat::gui
