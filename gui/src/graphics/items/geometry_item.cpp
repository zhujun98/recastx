/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/geometry_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

GeometryItem::GeometryItem(Scene &scene) : GraphicsItem(scene) {
    scene.addItem(this);
}

GeometryItem::~GeometryItem() = default;

void GeometryItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void GeometryItem::renderIm() {
//    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "GEOMETRY");
//
//    ImGui::Separator();
}

void GeometryItem::onFramebufferSizeChanged(int /* width */, int /* height */) {}

void GeometryItem::renderGl() {
}

} // namespace recastx::gui
