/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/axes_item.hpp"
#include "graphics/axes.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

AxesItem::AxesItem(Scene &scene) : GraphicsItem(scene), axes_(new Axes) {
    scene.addItem(this);
}

AxesItem::~AxesItem() = default;

void AxesItem::renderIm() {
    ImGui::Checkbox("Show axes", &visible_);
}

void AxesItem::onFramebufferSizeChanged(int /* width */, int /* height */) {}

void AxesItem::renderGl() {
    if (!visible_) return;

    axes_->render(scene_.viewMatrix(),
                  scene_.projectionMatrix(),
                  scene_.cameraDistance());
}

} // namespace recastx::gui
