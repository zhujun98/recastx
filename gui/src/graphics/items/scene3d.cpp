#include <algorithm>

#include <spdlog/spdlog.h>

#include "graphics/items/axes_item.hpp"
#include "graphics/items/statusbar_item.hpp"
#include "graphics/items/recon_item.hpp"
#include "graphics/items/scene3d.hpp"
#include "graphics/items/camera.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client) : Scene(client) {
    viewports_.emplace_back(std::make_unique<Viewport>());
    camera_ = std::make_unique<Camera>();
    this->addComponent(std::make_shared<AxesItem>(*this));
    this->addComponent(std::make_shared<StatusbarItem>(*this));
    this->addComponent(std::make_shared<ReconItem>(*this));
}

Scene3d::~Scene3d() = default;

void Scene3d::onFrameBufferSizeChanged(int width, int height) {
    viewports_[0]->update(0, 0, width, height);
}

} // tomcat::gui
