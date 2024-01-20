/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/statusbar_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"

namespace recastx::gui {

StatusbarItem::StatusbarItem(Scene& scene) : GraphicsItem(scene) {
    scene.addItem(this);
}

StatusbarItem::~StatusbarItem() = default;

void StatusbarItem::onWindowSizeChanged(int width, int /*height*/) {
    const auto& l = scene_.layout();
    size_ = { Style::STATUS_BAR_WIDTH * (float)width, float(l.bh) };
    pos_ = { static_cast<float>(2 * l.mw + l.lw), static_cast<float>(l.h - l.bh - l.mh) };
}

void StatusbarItem::renderIm() {
    if (!visible_) return;

    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::Begin("Status bar", NULL, ImGuiWindowFlags_NoDecoration);

    auto& io = ImGui::GetIO();
    ImGui::Text("GUI FPS: %.1f", io.Framerate);
    ImGui::Text("Volume frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("volumeUpdateFrameRate")));
    ImGui::Text("Slice frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("sliceUpdateFrameRate")));
    ImGui::Text("Projection frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("projectionUpdateFrameRate")));
    ImGui::End();
}

} // namespace recastx::gui