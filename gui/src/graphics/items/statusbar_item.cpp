#include <imgui.h>

#include "graphics/items/statusbar_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

StatusbarItem::StatusbarItem(Scene& scene)
        : GraphicsDataItem(scene) {
    scene.addItem(this);
}

StatusbarItem::~StatusbarItem() = default;

void StatusbarItem::onWindowSizeChanged(int width, int height) {
    size_ = {
        Style::BOTTOM_PANEL_WIDTH * (float)width,
        Style::BOTTOM_PANEL_HEIGHT * (float)height
    };

    pos_ = {
        Style::MARGIN + Style::LEFT_PANEL_WIDTH * (float)width + Style::SPACING,
        (float)(height) - size_[1] - Style::MARGIN
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