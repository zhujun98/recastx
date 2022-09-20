#include <iostream>

#include "imgui.h"

#include "graphics/nodes/axes_component.hpp"
#include "graphics/nodes/scene_camera3d.hpp"

namespace tomcat::gui {

AxesComponent::AxesComponent(Scene &scene) : StaticSceneComponent(scene) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    static const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, // x
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, // y
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, // z
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

void AxesComponent::renderIm(int /*width*/, int /*height*/) {
    ImGui::Checkbox("Show axes", &visible_);
}

void AxesComponent::renderGl(const glm::mat4& world_to_screen) {
    if (!visible_) return;

    // TODO draw axes on screen, should have access to camera here
    shader_->use();

    auto bottom_right_translate =
        glm::translate(glm::vec3(0.75f, -0.75f, 0.0f)) * glm::scale(glm::vec3(0.5f));
    shader_->setMat4("transform_matrix", bottom_right_translate * world_to_screen);

    glBindVertexArray(vao_);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // tomcat::gui
