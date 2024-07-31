/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/scan_component.hpp"

namespace recastx::gui {

ScanComponent::ScanComponent(RpcClient* client)
    : Component(client),
      scan_mode_(rpc::ScanMode_Mode_STATIC),
      scan_update_interval_(K_MIN_SCAN_UPDATE_INTERVAL) {
}

ScanComponent::~ScanComponent() = default;

void ScanComponent::draw(rpc::ServerState_State state) {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("SCAN MODE", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginDisabled(state == rpc::ServerState_State_PROCESSING);

        static int scan_mode = static_cast<int>(scan_mode_);
        ImGui::RadioButton("Static##SCAN_COMP", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_STATIC));
        ImGui::SameLine();
        ImGui::RadioButton("Dynamic##SCAN_COMP", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_DYNAMIC));
        ImGui::SameLine();
        ImGui::RadioButton("Continuous##SCAN_COMP", &scan_mode, static_cast<int>(rpc::ScanMode_Mode_CONTINUOUS));
        scan_mode_ = rpc::ScanMode_Mode(scan_mode);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Update interval");
        ImGui::SameLine();

        ImGui::BeginDisabled(scan_mode_ != rpc::ScanMode_Mode_CONTINUOUS);
        if (ImGui::ArrowButton("##SCAN_COMP_ARROW_LEFT", ImGuiDir_Left)) {
            assert(scan_update_interval_ >= K_SCAN_UPDATE_INTERVAL_STEP_SIZE);
            scan_update_interval_ -= K_SCAN_UPDATE_INTERVAL_STEP_SIZE;
            if (scan_update_interval_ < K_MIN_SCAN_UPDATE_INTERVAL) {
                scan_update_interval_ = K_MIN_SCAN_UPDATE_INTERVAL;
            }
        }
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::SameLine(0.0f, spacing);
        if (ImGui::ArrowButton("##SCAN_COMP_ARROW_RIGHT", ImGuiDir_Right)) {
            scan_update_interval_ += K_SCAN_UPDATE_INTERVAL_STEP_SIZE;
            if (scan_update_interval_ > K_MAX_SCAN_UPDATE_INTERVAL) {
                scan_update_interval_ = K_MAX_SCAN_UPDATE_INTERVAL;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%d", scan_update_interval_);
        ImGui::EndDisabled();

        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor();
}

RpcClient::State ScanComponent::updateServerParams() const {
    return client_->setScanMode(scan_mode_, scan_update_interval_);
}

} // namespace recastx::gui