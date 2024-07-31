/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/light_manager.hpp"
#include "graphics/widgets/light_widget.hpp"

namespace recastx::gui {

LightManager::LightManager() = default;

LightManager::~LightManager() = default;

std::shared_ptr<Light> LightManager::addLight() {
    auto light = std::make_shared<Light>();

    lights_.push_back(light);
    widgets_.push_back(std::make_unique<LightWidget>(light));

    return light;
}

void LightManager::renderGUI() {
    if (!widgets_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, Style::COLLAPSING_HEADER_COLOR);
        if (ImGui::CollapsingHeader("LIGHTS", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& widget : widgets_) widget->draw();
        }
        ImGui::PopStyleColor();
    }
}

} // namespace recastx::gui