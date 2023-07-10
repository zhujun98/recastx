/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/projection_item.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

ProjectionItem::ProjectionItem(Scene& scene) : GraphicsItem(scene) {
    scene.addItem(this);
}

ProjectionItem::~ProjectionItem() = default;

void ProjectionItem::renderIm() {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PREPROCESSING");

    // Projection downsampling
    ImGui::BeginDisabled(state_ == ServerState_State::ServerState_State_PROCESSING);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Downsampling:");
    ImGui::SameLine();

    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::Text("Col");
    ImGui::SameLine();
    if (ImGui::ArrowButton("##col_left", ImGuiDir_Left)) {
        if (downsampling_col_ > 1) {
            downsampling_col_--;
        }
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##col_right", ImGuiDir_Right)) {
        if (downsampling_col_ < 10) {
            downsampling_col_++;
        }
    }
    ImGui::SameLine();
    ImGui::Text("%d", downsampling_col_);
    ImGui::SameLine();

    ImGui::Text("Row");
    ImGui::SameLine();
    if (ImGui::ArrowButton("##row_left", ImGuiDir_Left)) {
        if (downsampling_row_ > 1) {
            downsampling_row_--;
        }
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##row_right", ImGuiDir_Right)) {
        if (downsampling_row_ < 10) {
            downsampling_row_++;
        }
    }
    ImGui::SameLine();
    ImGui::Text("%d", downsampling_row_);

    ImGui::EndDisabled();

    // Projection center adjustment
    ImGui::BeginDisabled(state_ == ServerState_State::ServerState_State_PROCESSING);
    ImGui::DragFloat("X offset", &x_offset_, 1, -50, 50, "%.1f");
    ImGui::DragFloat("Y offset", &y_offset_, 1, -50, 50, "%.1f");
    ImGui::EndDisabled();
}

bool ProjectionItem::updateServerParams() {
    return setDownsamplingParams();
}

bool ProjectionItem::setDownsamplingParams() {
    return scene_.client()->setDownsamplingParams(downsampling_col_, downsampling_row_);
}

} // namespace recastx::gui
