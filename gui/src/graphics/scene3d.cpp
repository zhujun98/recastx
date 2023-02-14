#include <algorithm>

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

#include "graphics/scene3d.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/style.hpp"
#include "graphics/items/axiscube_item.hpp"
#include "graphics/items/axes_item.hpp"
#include "graphics/items/icon_item.hpp"
#include "graphics/items/statusbar_item.hpp"
#include "graphics/items/recon_item.hpp"

namespace tomcat::gui {

Scene3d::Scene3d(MessageClient* client)
        : Scene(client),
          viewport_(new Viewport()),
          viewport_icon_(new Viewport(false)),
          viewport_axiscube_(new Viewport(false)),
          axes_item_(new AxesItem(*this)),
          icon_item_(new IconItem(*this)),
          recon_item_(new ReconItem(*this)),
          statusbar_item_(new StatusbarItem(*this)),
          axiscube_item_(new AxiscubeItem(*this)) {
    camera_ = std::make_unique<Camera>();
}

Scene3d::~Scene3d() = default;

void Scene3d::init() {
    Scene::init();

    scene_status_["tomoUpdateFrameRate"] = 0.;
}

void Scene3d::onFrameBufferSizeChanged(int width, int height) {
    viewport_->update(0, 0, width, height);

    int h = static_cast<int>(Style::TOP_PANEL_HEIGHT * (float)height);
    int w = h;
    viewport_axiscube_->update(width - w - int(Style::MARGIN * (float)width),
                               height - h - int(Style::MARGIN * (float)height),
                               w,
                               h);

    w = static_cast<int>(Style::ICON_WIDTH * (float)width);
    h = static_cast<int>(Style::ICON_HEIGHT* (float)height);
    viewport_icon_->update(int(Style::MARGIN * (float)width),
                           height - h - int(Style::MARGIN * (float)height),
                           w,
                           h);
}

void Scene3d::render() {
    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoResize);
    // 2/3 of the space for widget and 1/3 for labels
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);

    ImGui::Checkbox("Fix camera", &fixed_camera_);
    if (ImGui::Button("X-Y")) {
        camera_->setTopView();
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        camera_->setFrontView();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        camera_->setSideView();
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

    viewport_->use();
    GraphicsItem::RenderParams params;
    params["distance"] = camera_->distance();
    const auto& view = camera_->matrix();
    const auto& projection = viewport_->projection();
    axes_item_->renderGl(view, projection, params);
    recon_item_->renderGl(view, projection, params);
    statusbar_item_->renderGl(view, projection, params);

    viewport_icon_->use();
    params["aspectRatio"] = viewport_icon_->aspectRatio();
    icon_item_->renderGl(view, viewport_icon_->projection(), params);

    viewport_axiscube_->use();
    params["aspectRatio"] = viewport_axiscube_->aspectRatio();
    axiscube_item_->renderGl(view, viewport_axiscube_->projection(), params);
}

} // tomcat::gui
