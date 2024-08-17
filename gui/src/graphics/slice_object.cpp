/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/slice_object.hpp"
#include "graphics/renderer.hpp"
#include "graphics/light_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/style.hpp"
#include "common/utils.hpp"

namespace recastx::gui {

SliceObject::SliceObject() {

    auto vertex_shader =
#include "shaders/recon_sli.vert"
    ;
    auto fragment_shader =
#include "shaders/recon_sli.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);
    shader_->use();
    shader_->setInt("sliceTexture", 0);
    shader_->setInt("lutColor", 1);
    shader_->setInt("lutAlpha", 2);
    shader_->setInt("volumeTexture", 3);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    init();
}

SliceObject::~SliceObject() {
    glDeleteBuffers(1, &VBO_);
    glDeleteVertexArrays(1, &VAO_);
}

void SliceObject::init() {
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
}

void SliceObject::render(Renderer* renderer) {
    shader_->use();

    shader_->setMat4("model", model());
    shader_->setMat4("mvp", renderer->vpMatrix() * model());

    shader_->setVec3("sliceNormal", normal_);
    shader_->setVec3("viewPos", renderer->viewPos());

    auto &lights = renderer->lightManager()->lights();
    shader_->setInt("numLights", lights.size());
    for (size_t i = 0; i < lights.size(); ++i) {
        auto ptr = lights[i];
        shader_->setVec3("lights[" + std::to_string(i) + "].direction", ptr->direction());
        shader_->setVec3("lights[" + std::to_string(i) + "].ambient", ptr->ambient());
        shader_->setVec3("lights[" + std::to_string(i) + "].diffuse", ptr->diffuse());
        shader_->setVec3("lights[" + std::to_string(i) + "].specular", ptr->specular());
    }

    bool sample_volume = true;
    shader_->setInt("sampleVolumeTexture", sample_volume);

    auto mat = MaterialManager::instance().getMaterial<TransferFunc>(mat_id_);
    auto [min_v, max_v] = mat->minMaxVals();
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    intensity_.bind(0);
    mat->bind(1, 2);
    voxel_intensity_->bind(3);
    glBindVertexArray(VAO_);
    if (highlighting_ || hovering_ || selected_) {
        shader_->setBool("drawFrame", true);
        shader_->setVec4("frameColor", Style::K_HIGHLIGHTED_FRAME_COLOR);

#ifndef __APPLE__
        glLineWidth(2.f);
#endif
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    bool has_data = (sample_volume_ && voxel_intensity_->initialized()) || intensity_.initialized();
    if (has_data) {
        shader_->setBool("drawFrame", false);
        shader_->setBool("sampleVolume", !intensity_.initialized());
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else {
        shader_->setBool("drawFrame", true);
        shader_->setVec4("frameColor", Style::K_EMPTY_FRAME_COLOR);
#ifndef __APPLE__
        glLineWidth(2.f);
#endif
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    voxel_intensity_->unbind();
    mat->unbind();
    intensity_.unbind();
}

void SliceObject::setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y) {
    intensity_.invalidate();

    float orientation[16] = {   x.x,     x.y,    x.z,    0.f,
                                y.x,     y.y,    y.z,    0.f,
                                base.x,  base.y, base.z, 0.f,
                                0.f,     0.f,    0.f,    1.f};
    rotation_ = glm::make_mat4(orientation);
    normal_ = glm::normalize(glm::cross(x, y));
}

void SliceObject::setOffset(float offset) {
    intensity_.invalidate();
    pos_ = offset * normal_;
}

bool SliceObject::mouseHoverEvent(const MouseHoverEvent &ev) {
    if (ev.entering) hovering_ = true;
    else if (ev.exiting) hovering_ = false;
    return true;
}

} // recastx::gui
