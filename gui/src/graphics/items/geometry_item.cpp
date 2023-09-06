/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/geometry_item.hpp"
#include "graphics/axes.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

GeometryItem::GeometryItem(Scene &scene) : GraphicsItem(scene), axes_(new Axes) {
    scene.addItem(this);
}

GeometryItem::~GeometryItem() = default;

void GeometryItem::renderIm() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "GEOMETRY");

    ImGui::Checkbox("Show axes", &visible_);

    ImGui::Separator();

}

void GeometryItem::onFramebufferSizeChanged(int /* width */, int /* height */) {}

void GeometryItem::renderGl() {
    if (!visible_) return;

    axes_->render(scene_.viewMatrix(),
                  scene_.projectionMatrix(),
                  scene_.cameraDistance());
}

} // namespace recastx::gui
