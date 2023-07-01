/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

void StatusbarItem::onWindowSizeChanged(int width, int height) {
    size_ = {
        Style::STATUS_BAR_WIDTH * (float)width,
        Style::STATUS_BAR_HEIGHT * (float)height
    };

    pos_ = {
        (2.f * Style::MARGIN + Style::LEFT_PANEL_WIDTH) * (float)width,
        (1.f - Style::STATUS_BAR_HEIGHT - Style::MARGIN) * (float)(height)
    };
}

void StatusbarItem::renderIm() {
    ImGui::Checkbox("Show status bar", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Status bar", NULL, ImGuiWindowFlags_NoDecoration);

        auto& io = ImGui::GetIO();
        ImGui::Text("GUI FPS: %.1f", io.Framerate);
        ImGui::Text("Tomogram (update) FPS: %.1f",
                    std::any_cast<double>(scene_.getStatus("tomoUpdateFrameRate")));

        ImGui::End();

        ImGui::PopStyleColor();
    }
}

} // namespace recastx::gui