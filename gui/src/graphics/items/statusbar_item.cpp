#include <imgui.h>

#include "graphics/items/statusbar_item.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

StatusbarItem::StatusbarItem(Scene& scene)
    : GraphicsDataItem(scene) {}

StatusbarItem::~StatusbarItem() = default;

void StatusbarItem::onWindowSizeChanged(int width, int height) {
    pos_ = {
        Style::IMGUI_WINDOW_MARGIN + Style::IMGUI_CONTROL_PANEL_WIDTH + Style::IMGUI_WINDOW_SPACING,
        static_cast<float>(height) - Style::IMGUI_BOTTOM_PANEL_HEIGHT - Style::IMGUI_WINDOW_MARGIN
    };
    size_ = {
        static_cast<float>(width) - pos_[0] - Style::IMGUI_WINDOW_MARGIN,
        Style::IMGUI_BOTTOM_PANEL_HEIGHT
    };
}

void StatusbarItem::renderIm() {
    ImGui::Checkbox("Show status bar", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Status bar", NULL, ImGuiWindowFlags_NoDecoration);
        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void StatusbarItem::renderGl(const glm::mat4& /*view*/,
                             const glm::mat4& /*projection*/,
                             const RenderParams& /*params*/) {}

bool StatusbarItem::consume(const tomcat::PacketDataEvent& /*data*/) {
    return true;
}

} // namespace tomcat::gui