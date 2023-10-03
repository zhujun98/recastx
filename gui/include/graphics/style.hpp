/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_STYLE_HPP
#define GUI_STYLE_HPP

#include <imgui.h>

namespace recastx::gui {

struct Style {
    static constexpr float MARGIN = 0.01f;
    static constexpr float ICON_HEIGHT = 0.15f;
    static constexpr float ICON_WIDTH = 0.22f;
    static constexpr float LEFT_PANEL_WIDTH = ICON_WIDTH;
    static constexpr float STATUS_BAR_WIDTH = 0.28f;
    static constexpr float STATUS_BAR_HEIGHT = 0.18f;
    static constexpr float LOGGING_WIDTH = 0.46f;
    static constexpr float LOGGING_HEIGHT = 0.18f;
    static constexpr float TOP_PANEL_HEIGHT = 0.15f;
    static constexpr float PROJECTION_WIDTH = 0.20f;
    static constexpr float PROJECTION_HEIGHT = 0.30f;

    static void init() {
        ImGui::StyleColorsDark();

        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg]         = ImVec4(0.06f, 0.06f, 0.06f, 0.05f);
        colors[ImGuiCol_Border]           = ImVec4(0.43f, 0.43f, 0.50f, 0.00f);
        colors[ImGuiCol_FrameBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        colors[ImGuiCol_Header]           = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_TitleBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
        colors[ImGuiCol_CheckMark]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_Button]           = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
        colors[ImGuiCol_Separator]        = ImVec4(0.90f, 0.90f, 0.90f, 0.60f);
        colors[ImGuiCol_SliderGrab]       = ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
    }

};

} // namespace recastx::gui

#endif //GUI_STYLE_HPP
