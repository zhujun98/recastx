/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/preproc_item.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

PreprocItem::PreprocItem(Scene& scene)
        : GraphicsItem(scene), ramp_filter_name_("shepp") {
    scene.addItem(this);
}

PreprocItem::~PreprocItem() = default;

void PreprocItem::renderIm() {
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PREPROCESSING");

    ImGui::BeginDisabled(state_ == rpc::ServerState_State::ServerState_State_PROCESSING ||
                         state_ == rpc::ServerState_State::ServerState_State_ACQUIRING);

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

    ImGui::BeginDisabled(state_ == rpc::ServerState_State::ServerState_State_PROCESSING);

    // TODO: implement reconstruction with offsets
//    ImGui::DragFloat("X offset", &x_offset_, 1, -50, 50, "%.1f");
//    ImGui::DragFloat("Y offset", &y_offset_, 1, -50, 50, "%.1f");

    ImGui::Text("Ramp filter: ");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##RampFilter", filter_options.at(ramp_filter_name_).c_str())) {
        for (const auto& [k, v] : filter_options) {
            const bool is_selected = (ramp_filter_name_ == k);
            if (ImGui::Selectable(v.c_str(), is_selected)) {
                ramp_filter_name_ = k;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::EndDisabled();
}

bool PreprocItem::updateServerParams() {
    return setDownsampling() || setRampFilter();
}

bool PreprocItem::setDownsampling() {
    return scene_.client()->setDownsampling(downsampling_col_, downsampling_row_);
}

bool PreprocItem::setRampFilter() {
    return scene_.client()->setRampFilter(ramp_filter_name_);
}

} // namespace recastx::gui