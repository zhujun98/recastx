#ifndef GUI_STYLE_HPP
#define GUI_STYLE_HPP

namespace tomcat::gui {

struct Style {
    static constexpr float IMGUI_WINDOW_MARGIN = 10.f;
    static constexpr float IMGUI_WINDOW_SPACING = 10.f;
    static constexpr float IMGUI_ICON_HEIGHT = 80.f;
    static constexpr float IMGUI_CONTROL_PANEL_WIDTH = 250.f;
    static constexpr float IMGUI_TOP_PANEL_HEIGHT = 100.f;
    static constexpr float IMGUI_BOTTOM_PANEL_HEIGHT = 150.f;
};

} // namespace tomcat::gui

#endif //GUI_STYLE_HPP
