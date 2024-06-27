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
#include "graphics/style.hpp"

namespace recastx::gui {

GeometryItem::GeometryItem(Scene &scene)
        : GraphicsItem(scene),
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
    scene.addItem(this);
}

GeometryItem::~GeometryItem() = default;

void GeometryItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void GeometryItem::renderIm() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "GEOMETRY");

    ImGui::BeginDisabled();
    ImGui::RadioButton("Parallel", &beam_shape_, static_cast<int>(BeamShape::PARALELL));
    ImGui::SameLine();
    ImGui::RadioButton("Cone", &beam_shape_, static_cast<int>(BeamShape::CONE));
    ImGui::EndDisabled();

    auto state = scene_.serverState();
    ImGui::BeginDisabled(state & rpc::ServerState_State_PROCESSING);

    ImGui::DragInt("Column Count##GEOMETRY", &col_count_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragInt("Row Count##GEOMETRY", &row_count_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);

    ImGui::DragInt("Angle Count##GEOMETRY", &angle_count_, 10, 0, 4000, "%i", ImGuiSliderFlags_AlwaysClamp);
    ImGui::RadioButton("180 degree", &angle_range_, static_cast<int>(AngleRange::HALF));
    ImGui::SameLine();
    ImGui::RadioButton("360 degree", &angle_range_, static_cast<int>(AngleRange::FULL));

    ImGui::DragInt("Slice Size##GEOMETRY", &slice_size_, 16, 0, 4096, "%i", ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragInt("Volume Size##GEOMETRY", &volume_size_, 16, 0, 1024, "%i", ImGuiSliderFlags_AlwaysClamp);

    ImGui::DragIntRange2("X range##GEOMETRY", &x_[0], &x_[1]);
    ImGui::DragIntRange2("Y range##GEOMETRY", &y_[0], &y_[1]);
    ImGui::DragIntRange2("Z range##GEOMETRY", &z_[0], &z_[1]);

    ImGui::EndDisabled();

    ImGui::Separator();
}

void GeometryItem::onFramebufferSizeChanged(int /* width */, int /* height */) {}

void GeometryItem::renderGl() {
}

bool GeometryItem::updateServerParams() {
    return scene_.client()->setProjectionGeometry(
            static_cast<int>(beam_shape_),
            col_count_, row_count_, 1.f, 1.f, 1.f, 1.f,
            angle_count_, static_cast<int>(angle_range_)) ||
      scene_.client()->setReconGeometry(
              slice_size_, volume_size_, {x_[0], x_[1]}, {y_[0], y_[1]}, {z_[0], z_[1]});

}

} // namespace recastx::gui
