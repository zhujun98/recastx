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

void StatusbarItem::onWindowSizeChanged(int /*width*/, int /*height*/) {
    const auto& l = scene_.layout();
    pos_ = { static_cast<float>(2 * l.mw + l.lw), static_cast<float>(l.th + 2 * l.mh) };
    size_ = { static_cast<float>(l.lw), static_cast<float>(l.th) };
}

void StatusbarItem::renderIm() {
    if (!visible_) return;

    ImGui::SetNextWindowPos(pos_);
    ImGui::SetNextWindowSize(size_);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));

    ImGui::Begin("Status bar", NULL, ImGuiWindowFlags_NoDecoration);

    auto& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Volume frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("volumeUpdateFrameRate")));
    ImGui::Text("Slice frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("sliceUpdateFrameRate")));
    ImGui::Text("Projection frame rate: %.1f Hz",
                std::any_cast<double>(scene_.getStatus("projectionUpdateFrameRate")));
    ImGui::End();

    ImGui::PopStyleColor(2);
}

} // namespace recastx::gui