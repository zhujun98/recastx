#include <imgui.h>

#include "graphics/items/projection_item.hpp"
#include "graphics/scene.hpp"

namespace tomcat::gui {

ProjectionItem::ProjectionItem(Scene& scene)
        : GraphicsDataItem(scene) {
    scene.addItem(this);
}

ProjectionItem::~ProjectionItem() = default;

void ProjectionItem::renderIm() {

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "PROJECTION");

    // Projection downsampling
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Downsampling factor:");
    ImGui::SameLine();

    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
        if (downsampling_factor_ > 1) downsampling_factor_--;
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
        if (downsampling_factor_ < 10) downsampling_factor_++;
    }
    ImGui::SameLine();
    ImGui::Text("%d", downsampling_factor_);

    // Projection center adjustment
    ImGui::DragFloat("X offset", &x_offset_, 1, -50, 50, "%.1f");
    ImGui::DragFloat("Y offset", &y_offset_, 1, -50, 50, "%.1f");
}

void ProjectionItem::renderGl(const glm::mat4& /*view*/,
                              const glm::mat4& /*projection*/,
                              const RenderParams& /*params*/) {}

bool ProjectionItem::consume(const ReconDataPacket& packet) {
    return true;
}

} // tomcat::gui
