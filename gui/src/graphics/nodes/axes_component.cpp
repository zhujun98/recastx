#include <iostream>

#include "imgui.h"

#include "graphics/nodes/axes_component.hpp"
#include "graphics/nodes/scene_camera3d.hpp"

namespace tomcat::gui {

AxesComponent::AxesComponent(Scene &scene) : StaticSceneComponent(scene) {
    glGenVertexArrays(1, &axes_vao_handle_);
    glBindVertexArray(axes_vao_handle_);

    static const GLfloat axes_data[] = {
        0.0f, 0.0f, 0.0f, // 1
        1.0f, 0.0f, 0.0f, // 2
        0.0f, 1.0f, 0.0f, // 3
        0.0f, 0.0f, 1.0f, // 4
    };

    static const GLuint axes_idx_data[] = {0, 1, 0, 2, 0, 3};

    axes_index_count_ = 6;
    glGenBuffers(1, &axes_index_handle_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axes_index_handle_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, axes_index_count_ * sizeof(GLuint),
                 axes_idx_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glGenBuffers(1, &axes_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, axes_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), axes_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    auto vert =
#include "../shaders/lines.vert"
      ;
    auto frag =
#include "../shaders/lines.frag"
      ;

    axes_program_ = std::make_unique<ShaderProgram>(vert, frag);
}

AxesComponent::~AxesComponent() = default;

void AxesComponent::describe() {
    ImGui::Checkbox("Show axes", &show_);
}

void AxesComponent::render(const glm::mat4& world_to_screen) {
    if (!show_) return;

    // TODO draw axes on screen, should have access to camera here
    axes_program_->use();

    auto bottom_right_translate =
        glm::translate(glm::vec3(0.75f, -0.75f, 0.0f)) * glm::scale(glm::vec3(0.2f));
    axes_program_->setMat4("transform_matrix", bottom_right_translate * world_to_screen);

    glBindVertexArray(axes_vao_handle_);
    glLineWidth(3.0f);
    glDrawElements(GL_LINES, axes_index_count_, GL_UNSIGNED_INT, nullptr);
}

} // tomcat::gui
