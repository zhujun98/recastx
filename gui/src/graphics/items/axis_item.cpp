/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/axis_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

AxisItem::AxisItem(Scene &scene)
        : GraphicsItem(scene) {
    scene.addItem(this);
    vp_ = std::make_shared<Viewport>(false);

    static constexpr GLfloat vertices[] = {
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // x
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // y
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f  // z
    };

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
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

AxisItem::~AxisItem() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void AxisItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

void AxisItem::renderIm() {}

void AxisItem::onFramebufferSizeChanged(int /*width*/, int /*height*/) {}

void AxisItem::renderGl() {
    vp_->use();

    shader_->use();
    shader_->setFloat("scale", scene_.cameraDistance());
    shader_->setMat4("view", scene_.viewMatrix());
    shader_->setMat4("projection", vp_->projection());

    glBindVertexArray(vao_);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // namespace recastx::gui
