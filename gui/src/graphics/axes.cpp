/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/axes.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

Axes::Axes() {

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
#include "shaders/axes.vert"
    ;
    auto frag =
#include "shaders/axes.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

Axes::~Axes() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Axes::render(const glm::mat4& view, const glm::mat4& projection, float scale) {
    shader_->use();

    shader_->setFloat("scale", scale);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    glBindVertexArray(vao_);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // namespace recastx::gui
