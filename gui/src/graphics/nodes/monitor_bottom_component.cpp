#include <imgui.h>

#include "graphics/nodes/monitor_bottom_component.hpp"
#include "graphics/nodes/scene.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

MonitorBottomComponent::MonitorBottomComponent(Scene& scene)
    : DynamicSceneComponent(scene) {}

MonitorBottomComponent::~MonitorBottomComponent() = default;

void MonitorBottomComponent::onWindowSizeChanged(int width, int height) {
    pos_ = {
        Style::IMGUI_WINDOW_MARGIN + Style::IMGUI_CONTROL_PANEL_WIDTH + Style::IMGUI_WINDOW_SPACING,
        static_cast<float>(height) - Style::IMGUI_BOTTOM_PANEL_HEIGHT - Style::IMGUI_WINDOW_MARGIN
    };
    size_ = {
        static_cast<float>(width) - pos_[0] - Style::IMGUI_WINDOW_MARGIN,
        Style::IMGUI_BOTTOM_PANEL_HEIGHT
    };
}

void MonitorBottomComponent::renderIm() {
    ImGui::Checkbox("Show bottom monitors", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Bottom panel", NULL, ImGuiWindowFlags_NoDecoration);
        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void MonitorBottomComponent::renderGl() {}

bool MonitorBottomComponent::consume(const tomcat::PacketDataEvent &data) {
    return true;
}

} // namespace tomcat::gui