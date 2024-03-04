/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/lamp_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

LampItem::LampItem(Scene& scene)
        : GraphicsItem(scene),
          show_(false),
          rel_pos_(true),
          pos_{0.f, 0.f, 0.f},
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
#include "../shaders/lamp.vert"
    ;
    auto frag =
#include "../shaders/lamp.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    genVertices(0.03, 20);
    init();
}

LampItem::~LampItem() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void LampItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void LampItem::renderIm() {
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

    ImGui::DragFloat3("Position##LAMP", &pos_.x, 0.02f, -10.f, 10.f, "%.2f", ImGuiSliderFlags_ClampOnInput);

    if (ImGui::ColorEdit3("Color##LAMP", &light_.color.x)) {
        light_.ambient = ambient_ * light_.color;
        light_.diffuse = diffuse_ * light_.color;
        light_.specular = specular_ * light_.color;
    }

    if (ImGui::DragFloat("Ambient##LAMP", &ambient_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        light_.ambient = ambient_ * light_.color;
    }
    if (ImGui::DragFloat("Diffuse##LAMP", &diffuse_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        light_.diffuse = diffuse_ * light_.color;
    }
    if (ImGui::DragFloat("Specular##LAMP", &specular_, 0.005f, 0.f, 1.f, "%.3f", ImGuiSliderFlags_ClampOnInput)) {
        light_.specular = specular_ * light_.color;
    }
//
    ImGui::EndDisabled();
}

void LampItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

void LampItem::preRenderGl() {
    light_.pos = pos_;
    if (rel_pos_) {
        light_.pos += scene_.cameraPosition();
    }
}

void LampItem::renderGl() {
    if (!show_) return;

    vp_->use();

    const glm::mat4 mvp = vp_->projection() * scene_.cameraMatrix();

    shader_->use();
    shader_->setVec4("center", mvp * glm::vec4(light_.pos, 1.f));
    shader_->setFloat("xScale", 1.f / vp_->aspectRatio());
    shader_->setVec3("color", light_.color);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertices_.size() / 2);

    glDisable(GL_DEPTH_TEST);
}

void LampItem::genVertices(float radius, int count) {
    float step = glm::radians(360.f) / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        vertices_.push_back(radius * glm::cos(step * i));
        vertices_.push_back(radius * glm::sin(step * i));
    }
}

void LampItem::init() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

} // namespace recastx::gui