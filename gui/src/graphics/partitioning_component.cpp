#include <cmath>
#include <iostream>

#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "graphics/partitioning_component.hpp"

namespace tomcat::gui {

inline glm::vec3 hue_to_rgb(float hue) {
    int h = (int)(hue * 360);
    auto c = 1.0f;
    auto x = c * (1.0f - glm::abs((std::fmod(h / 60.0f, 2.0f) - 1.0f)));

    if (h < 60) {
        return {c, x, 0};
    } else if (h < 120) {
        return {x, c, 0};
    } else if (h < 180) {
        return {0, c, x};
    } else if (h < 240) {
        return {0, x, c};
    } else if (h < 300) {
        return {x, 0, c};
    } else {
        return {c, 0, x};
    }
}

PartitioningComponent::PartitioningComponent(Scene& scene)
    : scene_(scene) {
    static const GLfloat cube[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    glGenVertexArrays(1, &cube_vao_handle_);
    glBindVertexArray(cube_vao_handle_);
    glGenBuffers(1, &cube_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 9 * 12 * sizeof(GLfloat), cube,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    const char* part_vert =
#include "../src/shaders/part.vert"
    ;
    const char* part_frag =
#include "../src/shaders/part.frag"
    ;
    part_program_ = std::make_unique<ShaderProgram>(part_vert, part_frag);
}

PartitioningComponent::~PartitioningComponent() = default;

void PartitioningComponent::describe() {
    ImGui::Checkbox("Show partitioning", &show_);
    ImGui::SliderFloat("Scale", &global_scale_, 0.5f, 1.0f);
}

void PartitioningComponent::draw(glm::mat4 world_to_screen) {
    if (!show_) return;

    if (parts_.empty()) return;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    auto draw_part = [&](auto& the_part) {
        part_program_->use();

        glm::mat4 object_matrix =
            glm::translate(0.5f * (the_part.max_pt + the_part.min_pt)) *
            glm::scale(global_scale_ * (the_part.max_pt - the_part.min_pt)) *
            glm::translate(-glm::vec3(0.5f));

        part_program_->setMat4("transform_matrix", world_to_screen * object_matrix);
        part_program_->setVec3("rgb_color", hue_to_rgb((float)the_part.id / parts_.size()));

        glBindVertexArray(cube_vao_handle_);
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
    };

    for (auto& the_part : parts_) {
        draw_part(the_part);
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

} // tomcat::gui
