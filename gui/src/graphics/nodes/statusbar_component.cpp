#include <imgui.h>

#include "graphics/nodes/statusbar_component.hpp"

namespace tomcat::gui {

StatusbarComponent::StatusbarComponent(Scene& scene)
    : DynamicSceneComponent(scene) {}

StatusbarComponent::~StatusbarComponent() = default;

void StatusbarComponent::renderIm(int width, int height) {
    float margin = 5;
    float bar_h = 40;
    ImGui::SetNextWindowPos(ImVec2(margin, static_cast<float>(height) - bar_h - margin));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width) - 2 * margin, bar_h));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
    ImGui::Begin("Window Name", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);

    ImGui::PopStyleVar();

    ImGui::End();
}

void StatusbarComponent::renderGl(const glm::mat4 &world_to_screen) {}

bool StatusbarComponent::consume(const tomcat::PacketDataEvent &data) {
    return true;
}

} // namespace tomcat::gui