#include <imgui.h>

#include "graphics/nodes/monitor_right_component.hpp"
#include "graphics/nodes/scene.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

MonitorRightComponent::MonitorRightComponent(Scene& scene)
    : DynamicSceneComponent(scene) {}

MonitorRightComponent::~MonitorRightComponent() = default;

void MonitorRightComponent::onWindowSizeChanged(int width, int height) {
    pos_ = {
        static_cast<float>(width) - Style::IMGUI_WINDOW_MARGIN - Style::IMGUI_RIGHT_PANEL_WIDTH,
        Style::IMGUI_WINDOW_MARGIN + Style::IMGUI_TOP_PANEL_HEIGHT + Style::IMGUI_WINDOW_SPACING
    };
    size_ = {
        Style::IMGUI_RIGHT_PANEL_WIDTH,
        static_cast<float>(height) - pos_[1]
            - Style::IMGUI_WINDOW_SPACING - Style::IMGUI_BOTTOM_PANEL_HEIGHT - Style::IMGUI_WINDOW_MARGIN
    };
}

void MonitorRightComponent::renderIm() {
    ImGui::Checkbox("Show right monitors", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Right panel", NULL, ImGuiWindowFlags_NoDecoration);
        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void MonitorRightComponent::renderGl() {}

bool MonitorRightComponent::consume(const tomcat::PacketDataEvent &data) {
    return true;
}

} // namespace tomcat::gui