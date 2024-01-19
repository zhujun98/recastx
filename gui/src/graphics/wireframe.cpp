/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/wireframe.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

Wireframe::Wireframe(const glm::vec4& color)
        : color_(color) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(primitives::wireframe_cube_indices),
                 primitives::wireframe_cube_indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::wireframe_cube),
                 primitives::wireframe_cube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    auto vert =
#include "shaders/wireframe_cube.vert"
    ;
    auto frag =
#include "shaders/wireframe_cube.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

Wireframe::~Wireframe() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
}

void Wireframe::render(const glm::mat4& view, const glm::mat4& projection) {
    shader_->use();
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    shader_->setVec4("color", color_);

    glBindVertexArray(vao_);
    glLineWidth(1.f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
}

} // namespace recastx::gui