#ifndef GUI_STYLE_HPP
#define GUI_STYLE_HPP

#include <imgui.h>

namespace tomcat::gui {

struct Style {
    static constexpr float IMGUI_WINDOW_MARGIN = 10.f;
    static constexpr float IMGUI_WINDOW_SPACING = 10.f;
    static constexpr float IMGUI_ICON_HEIGHT = 80.f;
    static constexpr float IMGUI_ICON_WIDTH = 250.f;
    static constexpr float IMGUI_CONTROL_PANEL_WIDTH = 250.f;
    static constexpr float IMGUI_TOP_PANEL_HEIGHT = 150.f;
    static constexpr float IMGUI_BOTTOM_PANEL_HEIGHT = 150.f;
    static constexpr float IMGUI_ROTATING_AXIS_WIDTH = 100.f;
    static constexpr float IMGUI_RIGHT_PANEL_WIDTH = 250.f;

    static void init() {
        ImGui::StyleColorsDark();

        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg]         = ImVec4(0.06f, 0.06f, 0.06f, 0.05f);
        colors[ImGuiCol_Border]           = ImVec4(0.43f, 0.43f, 0.50f, 0.00f);
        colors[ImGuiCol_FrameBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        colors[ImGuiCol_TitleBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
        colors[ImGuiCol_CheckMark]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_Button]           = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
        colors[ImGuiCol_Separator]        = ImVec4(0.90f, 0.90f, 0.90f, 0.60f);

    }

};

} // namespace tomcat::gui

#endif //GUI_STYLE_HPP
