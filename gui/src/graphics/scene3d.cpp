#include <algorithm>

#include <spdlog/spdlog.h>

#include "graphics/scene3d.hpp"
#include "graphics/camera.hpp"
#include "graphics/items/axes_item.hpp"
#include "graphics/items/statusbar_item.hpp"
#include "graphics/items/recon_item.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(Client* client)
        : Scene(client),
          axes_item_(new AxesItem(*this)),
          recon_item_(new ReconItem(*this)),
          statusbar_item_(new StatusbarItem(*this)) {
    camera_ = std::make_unique<Camera>();
    viewports_.emplace_back(std::make_unique<Viewport>());
}

Scene3d::~Scene3d() = default;

void Scene3d::onFrameBufferSizeChanged(int width, int height) {
    viewports_[0]->update(0, 0, width, height);
}

void Scene3d::render() {
    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoResize);
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::Checkbox("Fix camera", &fixed_camera_);
    if (ImGui::Button("X-Y")) {
        camera_->setFrontView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        camera_->setSideView();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        camera_->setTopView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective")) {
        camera_->setPerspectiveView();
    }

    axes_item_->renderIm();
    ImGui::Separator();
    recon_item_->renderIm();
    ImGui::Separator();
    statusbar_item_->renderIm();

    ImGui::End();

    GraphicsItem::RenderParams params;
    params["distance"] = camera_->distance();
    viewports_[0]->use();
    for (auto &item : items_) {
        item->renderGl(camera_->matrix(), viewports_[0]->projection(), params);
    }
}

} // tomcat::gui
