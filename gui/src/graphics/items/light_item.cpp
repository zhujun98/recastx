/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/items/light_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

LightItem::LightItem(Scene& scene)
        : GraphicsItem(scene),
          show_(false),
          rel_pos_(true),
          pos_{-3.f, 1.f, -3.f},
          ambient_(0.5f),
          diffuse_(0.7f),
          specular_(1.0f) {
    scene.addItem(this);
    vp_ = std::make_shared<Viewport>();

    light_.is_enabled = true;

    glm::vec3 color = {1.f, 1.f, 1.f};
    light_.color = color;
    light_.ambient = ambient_ * color;
    light_.diffuse = diffuse_ * color;
    light_.specular = specular_ * color;

    auto vert =
#include "../shaders/point_light.vert"
    ;
    auto frag =
#include "../shaders/point_light.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    genVertices(0.1);
    init();
}

LightItem::~LightItem() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void LightItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void LightItem::renderIm() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "LIGHTING");

    ImGui::Checkbox("On##LAMP", &light_.is_enabled);
    ImGui::BeginDisabled(!light_.is_enabled);

    ImGui::SameLine();
    ImGui::Checkbox("Show##LAMP", &show_);

    ImGui::SameLine();
    if (ImGui::Checkbox("Rel. Positioning##LAMP", &rel_pos_)) {
        const auto& cam_pos = scene_.cameraPosition();
        if (rel_pos_) {
            pos_ -= cam_pos;
        } else {
            pos_ += cam_pos;
        }
    }

    ImGui::DragFloat3("Position##LAMP", &pos_.x, 0.02f, -10.f, 10.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

    if (ImGui::ColorEdit3("Color##LAMP", &light_.color.x)) {
        light_.ambient = ambient_ * light_.color;
        light_.diffuse = diffuse_ * light_.color;
        light_.specular = specular_ * light_.color;
    }

    if (ImGui::DragFloat("Ambient##LAMP", &ambient_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        light_.ambient = ambient_ * light_.color;
    }
    if (ImGui::DragFloat("Diffuse##LAMP", &diffuse_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        light_.diffuse = diffuse_ * light_.color;
    }
    if (ImGui::DragFloat("Specular##LAMP", &specular_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        light_.specular = specular_ * light_.color;
    }

    ImGui::EndDisabled();
}

void LightItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

void LightItem::preRenderGl() {
    light_.pos = pos_;
    if (rel_pos_) {
        light_.pos += scene_.cameraPosition();
    }
}

void LightItem::renderGl() {
    if (!show_) return;

    vp_->use();

    shader_->use();
    shader_->setMat4("mvp", vp_->projection() * scene_.cameraMatrix() * glm::translate(glm::mat4(1.f), light_.pos));
    shader_->setVec3("color", light_.color);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao_);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices_.size()));

    glDisable(GL_DEPTH_TEST);
}

void LightItem::genVertices(float radius) {
    vertices_.emplace_back(-radius, 0.f, 0.f);
    vertices_.emplace_back( radius, 0.f, 0.f);
    vertices_.emplace_back(0.f, -radius, 0.f);
    vertices_.emplace_back(0.f,  radius, 0.f);
    vertices_.emplace_back(0.f, 0.f, -radius);
    vertices_.emplace_back(0.f, 0.f,  radius);
}

void LightItem::init() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float) * 3, vertices_.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

} // namespace recastx::gui