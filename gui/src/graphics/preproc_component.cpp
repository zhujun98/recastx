/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/preproc_component.hpp"

namespace recastx::gui {

PreprocComponent::PreprocComponent(RpcClient* client)
    : Component(client), ramp_filter_name_("shepp") {
}

PreprocComponent::~PreprocComponent() = default;

void PreprocComponent::draw(rpc::ServerState_State state) {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("PREPROCESSING", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginDisabled(state & rpc::ServerState_State_PROCESSING);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Downsample:");
        ImGui::SameLine();

        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::Text("Col");
        ImGui::SameLine();
        if (ImGui::ArrowButton("##PREPROC_COMP_C_ARROW_LEFT", ImGuiDir_Left)) {
            if (downsampling_col_ > 1) {
                downsampling_col_--;
            }
        }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##PREPROC_COMP_C_ARROW_RIGHT", ImGuiDir_Right)) {
            if (downsampling_col_ < 10) {
                downsampling_col_++;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%d", downsampling_col_);
        ImGui::SameLine();

        ImGui::Text("Row");
        ImGui::SameLine();
        if (ImGui::ArrowButton("##PREPROC_COMP_R_ARROW_LEFT", ImGuiDir_Left)) {
            if (downsampling_row_ > 1) {
                downsampling_row_--;
            }
        }
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##PREPROC_COMP_R_ARROW_RIGHT", ImGuiDir_Right)) {
            if (downsampling_row_ < 10) {
                downsampling_row_++;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%d", downsampling_row_);

        ImGui::Checkbox("Minus Log##PREPROC_COMP", &minus_log_);
        ImGui::DragInt("Offset##PREPROC_COMP", &offset_, 1, -100, 100);

        ImGui::AlignTextToFramePadding();
        if (ImGui::BeginCombo("Ramp filter##PREPROC_COMP", filter_options_.at(ramp_filter_name_).c_str())) {
            for (const auto &[k, v]: filter_options_) {
                const bool is_selected = (ramp_filter_name_ == k);
                if (ImGui::Selectable(v.c_str(), is_selected)) {
                    ramp_filter_name_ = k;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor();
}

RpcClient::State PreprocComponent::updateServerParams() const {
    CHECK_CLIENT_STATE(client_->setDownsampling(downsampling_col_, downsampling_row_))
    CHECK_CLIENT_STATE(client_->setCorrection(offset_, minus_log_))
    CHECK_CLIENT_STATE(client_->setRampFilter(ramp_filter_name_))
    return RpcClient::State::OK;
}

} // namespace recastx::gui