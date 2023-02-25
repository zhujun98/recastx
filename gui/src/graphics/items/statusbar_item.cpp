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
        (1.f - Style::LEFT_PANEL_WIDTH - 3.f * Style::MARGIN) * (float)width,
        (float)height
    };

    pos_ = {
        (2.f * Style::MARGIN + Style::LEFT_PANEL_WIDTH) * (float)width,
        (1.f - Style::BOTTOM_PANEL_HEIGHT - Style::MARGIN) * (float)(height)
    };
}

void StatusbarItem::renderIm() {
    ImGui::Checkbox("Show status bar", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Status bar", NULL, ImGuiWindowFlags_NoDecoration);

        auto& io = ImGui::GetIO();
        ImGui::Text("GUI FPS: %.1f", io.Framerate);
        ImGui::Text("Tomogram (update) FPS: %.1f",
                    std::any_cast<double>(scene_.getStatus("tomoUpdateFrameRate")));

        ImGui::End();

        ImGui::PopStyleColor();
    }
}

void StatusbarItem::renderGl(const glm::mat4& /*view*/,
                             const glm::mat4& /*projection*/,
                             const RenderParams& /*params*/) {}

bool StatusbarItem::consume(const tomcat::ReconDataPacket& /*data*/) {
    return true;
}

} // namespace tomcat::gui