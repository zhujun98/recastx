/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/widgets/light_widget.hpp"

namespace recastx::gui {

LightWidget::LightWidget(std::shared_ptr<Light> light)
        : Widget("Light " + std::to_string(light->id())),
          light_(std::move(light)) {
    id_ = "##LIGHT" + std::to_string(light_->id());
}

void LightWidget::draw() {
    ImGui::TextUnformatted(name_.c_str());


    ImGui::Checkbox(("Show" + id_).c_str(), &light_->visible_);

    ImGui::DragFloat3(("Position" + id_).c_str(), &light_->rel_pos_.x, 0.01f, -1.f, 1.f);

    ImGui::ColorEdit3(("Color" + id_).c_str(), &light_->color_.x);
    ImGui::SliderFloat(("Ambient" + id_).c_str(), &light_->ambient_, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(("Diffuse" + id_).c_str(), &light_->diffuse_, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SliderFloat(("Specular" + id_).c_str(), &light_->specular_, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
}

} // namespace recastx::gui