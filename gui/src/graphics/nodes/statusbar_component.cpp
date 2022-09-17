#include <imgui.h>

#include "graphics/nodes/statusbar_component.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

StatusbarComponent::StatusbarComponent(Scene& scene)
    : DynamicSceneComponent(scene) {}

StatusbarComponent::~StatusbarComponent() = default;

void StatusbarComponent::renderIm(int width, int height) {
    float x0 = Style::IMGUI_WINDOW_MARGIN + Style::IMGUI_CONTROL_PANEL_WIDTH + Style::IMGUI_WINDOW_SPACING;
    float w = static_cast<float>(width) - x0 - Style::IMGUI_WINDOW_MARGIN;
    float h = Style::IMGUI_TOP_PANEL_HEIGHT;
    float y0 = static_cast<float>(height) - h - Style::IMGUI_WINDOW_MARGIN;
    ImGui::SetNextWindowPos(ImVec2(x0, y0));
    ImGui::SetNextWindowSize(ImVec2(w, h));

    ImGui::Begin("Window Name", NULL, ImGuiWindowFlags_NoDecoration);

    ImGui::End();
}

void StatusbarComponent::renderGl(const glm::mat4 &world_to_screen) {}

bool StatusbarComponent::consume(const tomcat::PacketDataEvent &data) {
    return true;
}

} // namespace tomcat::gui