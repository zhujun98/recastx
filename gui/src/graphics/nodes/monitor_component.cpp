#include <imgui.h>

#include "graphics/nodes/monitor_component.hpp"
#include "graphics/nodes/scene.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

MonitorComponent::MonitorComponent(Scene& scene)
    : DynamicSceneComponent(scene) {}

MonitorComponent::~MonitorComponent() = default;

void MonitorComponent::renderIm() {
    ImGui::Checkbox("Show monitors", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        // bottom panel
        // ------------
        float x0 = Style::IMGUI_WINDOW_MARGIN
                + Style::IMGUI_CONTROL_PANEL_WIDTH
                + Style::IMGUI_WINDOW_SPACING;
        float w = static_cast<float>(scene_.width())
                - x0
                - Style::IMGUI_WINDOW_MARGIN;
        float h = Style::IMGUI_BOTTOM_PANEL_HEIGHT;
        float y0 = static_cast<float>(scene_.height())
                - Style::IMGUI_BOTTOM_PANEL_HEIGHT
                - Style::IMGUI_WINDOW_MARGIN;
        ImGui::SetNextWindowPos(ImVec2(x0, y0));
        ImGui::SetNextWindowSize(ImVec2(w, h));

        ImGui::Begin("Bottom panel", NULL, ImGuiWindowFlags_NoDecoration);

        ImGui::End();

        // right panel
        // -----------
        x0 = static_cast<float>(scene_.width())
                - Style::IMGUI_WINDOW_MARGIN
                - Style::IMGUI_RIGHT_PANEL_WIDTH;
        y0 = Style::IMGUI_WINDOW_MARGIN
                + Style::IMGUI_TOP_PANEL_HEIGHT
                + Style::IMGUI_WINDOW_SPACING;
        w = Style::IMGUI_RIGHT_PANEL_WIDTH;
        h = static_cast<float>(scene_.height())
                - y0
                - Style::IMGUI_WINDOW_SPACING
                - Style::IMGUI_BOTTOM_PANEL_HEIGHT
                - Style::IMGUI_WINDOW_MARGIN;
        ImGui::SetNextWindowPos(ImVec2(x0, y0));
        ImGui::SetNextWindowSize(ImVec2(w, h));

        ImGui::Begin("Right panel", NULL, ImGuiWindowFlags_NoDecoration);

        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void MonitorComponent::renderGl() {}

bool MonitorComponent::consume(const tomcat::PacketDataEvent &data) {
    return true;
}

} // namespace tomcat::gui