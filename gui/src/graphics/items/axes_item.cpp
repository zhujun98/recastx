/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <iostream>

#include <imgui.h>

#include "graphics/items/axes_item.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/primitives.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

AxesItem::AxesItem(Scene &scene) : GraphicsItem(scene) {
    scene.addItem(this);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::axes), primitives::axes, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "../shaders/axes.vert"
    ;
    auto frag =
#include "../shaders/axes.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

AxesItem::~AxesItem() = default;

void AxesItem::renderIm() {
    ImGui::Checkbox("Show axes", &visible_);
}

void AxesItem::renderGl(const glm::mat4& view,
                        const glm::mat4& projection,
                        const RenderParams& params) {
    if (!visible_) return;

    shader_->use();

    shader_->setFloat("scale", std::any_cast<float>(params.at("distance")));
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    glBindVertexArray(vao_);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // namespace recastx::gui
