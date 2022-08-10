#include <iostream>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "graphics/geom_component.hpp"
#include "graphics/primitives.hpp"
#include "scenes/scene_camera3d.hpp"

namespace gui {

GeomComponent::GeomComponent(Scene& scene) : scene_(scene) {
    current_projection_ = 0;

    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);
    glGenBuffers(1, &vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), square(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    const char* tex_vert =
#include "../src/shaders/show_texture.vert"
    ;
    const char* tex_frag =
#include "../src/shaders/show_texture.frag"
    ;
    program_ = std::make_unique<ShaderProgram>(tex_vert, tex_frag);

    const char* cube_vert =
#include "../src/shaders/wireframe_cube.vert"
    ;
    const char* cube_frag =
#include "../src/shaders/wireframe_cube.frag"
    ;
    cube_program_ = std::make_unique<ShaderProgram>(cube_vert, cube_frag);

    const char* beam_vert =
#include "../src/shaders/beam.vert"
    ;
    const char* beam_frag =
#include "../src/shaders/beam.frag"
    ;
    beam_program_ = std::make_unique<ShaderProgram>(beam_vert, beam_frag);

    glGenVertexArrays(1, &beam_vao_handle_);
    glBindVertexArray(beam_vao_handle_);
    glGenBuffers(1, &beam_vbo_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, beam_vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 9 * 12 * sizeof(GLfloat), alt_pyramid(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    speed_ = 0.0f;

    cm_texture_id_ = scene_.camera().colormapTextureId();
}

GeomComponent::~GeomComponent() = default;

void GeomComponent::describe() {
    ImGui::Checkbox("Show geometry", &show_);
    ImGui::SliderInt("Projection", &current_projection_, 0,
                     projections_.size() - 1);
    ImGui::SliderFloat("Speed (pps)", &speed_, 0.0f, 0.1f);

    recorder_.describe();
}

void GeomComponent::tick(float time_elapsed) {
    if (projections_.empty()) return;

    if (total_time_elapsed_ < 0.0f) {
        total_time_elapsed_ = 0.01f;
    } else {
        total_time_elapsed_ += speed_ * time_elapsed * projections_.size();
    }

    while (total_time_elapsed_ > 1.0f) {
        current_projection_ = (current_projection_ + 1) % projections_.size();
        total_time_elapsed_ -= 1.0f;
    }
}

void GeomComponent::draw(glm::mat4 world_to_screen) {
    if (!show_) {
        recorder_.capture();
        return;
    }

    if (projections_.empty()) return;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, cm_texture_id_);

    auto draw_projection = [&](auto& proj) {
        cube_program_->use();

        // DRAW SOURCE
        glm::mat4 object_matrix =
            glm::translate(proj.source_position) * glm::scale(glm::vec3(0.01f));

        auto transform_matrix = world_to_screen * object_matrix;

        cube_program_->setMat4("transform_matrix", transform_matrix);

        glBindVertexArray(cube_vao_handle_);
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        // DRAW PROJECTION
        program_->use();

        program_->setInt("texture_sampler", 0);
        proj.data_texture.bind();

        program_->setInt("colormap_sampler", 1);

        program_->setMat4("orientation_matrix", proj.detector_orientation);
        program_->setMat4("world_to_screen_matrix", world_to_screen);

        glBindVertexArray(vao_handle_);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // DRAW BEAM
        beam_program_->use();

        auto orientation = proj.detector_orientation;
        orientation[2][0] -= proj.source_position[0];
        orientation[2][1] -= proj.source_position[1];
        orientation[2][2] -= proj.source_position[2];
        auto beam_to_world =
            glm::translate(proj.source_position) * orientation;

        beam_program_->use();
        beam_program_->setMat4("transform_matrix", world_to_screen);
        beam_program_->setMat4("beam_matrix", beam_to_world);
        glBindVertexArray(beam_vao_handle_);

        glEnable(GL_CULL_FACE);
        glDrawArrays(GL_TRIANGLES, 0, 12 * 9);
        glDisable(GL_CULL_FACE);
        recorder_.capture();
    };

    draw_projection(projections_[current_projection_]);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

} // namespace gui
