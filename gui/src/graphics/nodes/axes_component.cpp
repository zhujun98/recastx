#include <iostream>

#include "imgui.h"

#include "graphics/nodes/axes_component.hpp"
#include "graphics/nodes/camera.hpp"

namespace tomcat::gui {

AxesComponent::AxesComponent(Scene &scene) : StaticSceneComponent(scene) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // TODO: draw infinite lines
    static const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, // x
        0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, // y
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, // z
    };

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    auto vert =
#include "../shaders/lines.vert"
      ;
    auto frag =
#include "../shaders/lines.frag"
      ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

AxesComponent::~AxesComponent() = default;

void AxesComponent::renderIm() {
    ImGui::Checkbox("Show axes", &visible_);
}

void AxesComponent::renderGl() {
    if (!visible_) return;

    // TODO draw axes on screen, should have access to camera here
    shader_->use();

    shader_->setMat4("view", scene_.camera().matrix());
    shader_->setMat4("projection", scene_.projection());

    glBindVertexArray(vao_);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // tomcat::gui
