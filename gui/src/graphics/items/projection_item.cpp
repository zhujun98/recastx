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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PROJECTION");

    // Projection downsampling
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Downsampling factor:");
    ImGui::SameLine();

    ImGui::BeginDisabled(state_ == StatePacket_State::StatePacket_State_PROCESSING);
    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
        if (downsampling_factor_ > 1) downsampling_factor_--;
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
        if (downsampling_factor_ < 10) downsampling_factor_++;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("%d", downsampling_factor_);

    // Projection center adjustment
    ImGui::BeginDisabled(state_ == StatePacket_State::StatePacket_State_PROCESSING);
    ImGui::DragFloat("X offset", &x_offset_, 1, -50, 50, "%.1f");
    ImGui::DragFloat("Y offset", &y_offset_, 1, -50, 50, "%.1f");
    ImGui::EndDisabled();
}

} // namespace recastx::gui
