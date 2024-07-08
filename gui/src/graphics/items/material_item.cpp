/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/material_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"

namespace recastx::gui {

MaterialItem::MaterialItem(Scene& scene)
        : GraphicsItem(scene),
          ambient_{0.24725f, 0.1995f, 0.0745f},
          diffuse_{0.75164f, 0.60648f, 0.22648f},
          specular_{0.628281f, 0.555802f, 0.366065f} {
    scene.addItem(this);

    material_.ambient = glm::vec3(ambient_[0], ambient_[1], ambient_[2]);
    material_.diffuse = glm::vec3(diffuse_[0], diffuse_[1], diffuse_[2]);
    material_.specular = glm::vec3(specular_[0], specular_[1], specular_[2]);
    material_.alpha = 1.f;
    material_.shininess = 51.2f;
}

MaterialItem::~MaterialItem() = default;

void MaterialItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void MaterialItem::renderIm() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "MATERIAL");

    if (ImGui::ColorEdit3("Ambient##MATERIAL", ambient_)) {
        material_.ambient = glm::vec3(ambient_[0], ambient_[1], ambient_[2]);
    }
    if (ImGui::ColorEdit3("Diffuse##MATERIAL", diffuse_)) {
        material_.diffuse = glm::vec3(diffuse_[0], diffuse_[1], diffuse_[2]);
    }
    if (ImGui::ColorEdit3("Specular##MATERIAL", specular_)) {
        material_.specular = glm::vec3(specular_[0], specular_[1], specular_[2]);
    }

    ImGui::DragFloat("Alpha##MATERIAL", &material_.alpha, 0.005f, 0.0f, 1.0f, "%.3f",
                     ImGuiSliderFlags_AlwaysClamp);
    ImGui::DragFloat("Shininess##MATERIAL", &material_.shininess, 1.f, 0.f, 100.f, "%.1f",
                     ImGuiSliderFlags_AlwaysClamp);
}

} // namespace recastx::gui