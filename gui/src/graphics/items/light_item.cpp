/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/light_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

LightItem::LightItem(Scene &scene)
        : GraphicsItem(scene), color_({1.f, 1.f, 1.f}), ambient_(0.5f), diffuse_(0.7f), specular_(1.f) {
    scene.addItem(this);

    light_.is_enabled = true;
    light_.pos = {0.f, 0.f, 0.f};
    light_.ambient = ambient_ * color_;
    light_.diffuse = diffuse_ * color_;
    light_.specular = specular_ * color_;
}

LightItem::~LightItem() = default;

void LightItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void LightItem::renderIm() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "LIGHTING");
    ImGui::Checkbox("On##Light", &light_.is_enabled);
    if (ImGui::SliderFloat("Ambient##Light", &ambient_, 0.f, 1.f)) {
        light_.ambient = ambient_ * color_;
    }
    if (ImGui::SliderFloat("Diffuse##Light", &diffuse_, 0.f, 1.f)) {
        light_.diffuse = diffuse_ * color_;
    }
    if (ImGui::SliderFloat("Specular##Light", &specular_, 0.f, 1.f)) {
        light_.specular = specular_ * color_;
    }
}

void LightItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

void LightItem::renderGl() {
    light_.pos = scene_.cameraPosition() - 2.f * scene_.viewDir();
}

} // namespace recastx::gui
