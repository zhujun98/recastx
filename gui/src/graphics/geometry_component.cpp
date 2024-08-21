/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/geometry_component.hpp"

namespace recastx::gui {

GeometryComponent::GeometryComponent(RpcClient* client)
        : Component(client),
          beam_shape_(static_cast<int>(BeamShape::PARALELL)),
          col_count_(0),
          row_count_(0),
          angle_count_(0),
          angle_range_(static_cast<int>(AngleRange::HALF)),
          slice_size_(0),
          volume_size_(0),
          x_{0, 0},
          y_{0, 0},
          z_{0, 0} {
}

GeometryComponent::~GeometryComponent() = default;

void GeometryComponent::draw(rpc::ServerState_State state) {
    ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
    if (ImGui::CollapsingHeader("GEOMETRY", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginDisabled();
        ImGui::RadioButton("Parallel##GEOM_COMP", &beam_shape_, static_cast<int>(BeamShape::PARALELL));
        ImGui::SameLine();
        ImGui::RadioButton("Cone##GEOM_COMP", &beam_shape_, static_cast<int>(BeamShape::CONE));
        ImGui::EndDisabled();

        ImGui::BeginDisabled(state & rpc::ServerState_State_PROCESSING);

        ImGui::DragInt("Column Count##GEOM_COMP", &col_count_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragInt("Row Count##GEOM_COMP", &row_count_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);

        ImGui::DragInt("Angle Count##GEOM_COMP", &angle_count_, 10, 0, 4000, "%i", ImGuiSliderFlags_AlwaysClamp);
        ImGui::RadioButton("180 degree##GEOM_COMP", &angle_range_, static_cast<int>(AngleRange::HALF));
        ImGui::SameLine();
        ImGui::RadioButton("360 degree##GEOM_COMP", &angle_range_, static_cast<int>(AngleRange::FULL));

        ImGui::DragInt("Slice Size##GEOM_COMP", &slice_size_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragInt("Volume Size##GEOM_COMP", &volume_size_, 16, 0, 1024, "%i", ImGuiSliderFlags_AlwaysClamp);

        ImGui::DragIntRange2("X range##GEOM_COMP", &x_[0], &x_[1]);
        ImGui::DragIntRange2("Y range##GEOM_COMP", &y_[0], &y_[1]);
        ImGui::DragIntRange2("Z range##GEOM_COMP", &z_[0], &z_[1]);

        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor();
}

RpcClient::State GeometryComponent::updateServerParams() const {
    CHECK_CLIENT_STATE(client_->setProjectionGeometry(static_cast<int>(beam_shape_),
                                                      col_count_,
                                                      row_count_,
                                                      1.f, 1.f, 1.f, 1.f,
                                                      angle_count_,
                                                      static_cast<int>(angle_range_)))
    CHECK_CLIENT_STATE(client_->setReconGeometry(slice_size_, volume_size_,
                                                 {x_[0], x_[1]}, {y_[0], y_[1]}, {z_[0], z_[1]}))
    return RpcClient::State::OK;
}

} // namespace recastx::gui
