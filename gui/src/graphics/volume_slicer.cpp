/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <glm/glm.hpp>

#include "graphics/volume_slicer.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

VolumeSlicer::VolumeSlicer(size_t num_slices)
        : shadow_(800, 800), num_slices_(num_slices), front_(0.f) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
}

VolumeSlicer::~VolumeSlicer() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void VolumeSlicer::update(const glm::vec3& view_dir, bool inverted) {
    if (slices_.size() != num_slices_ * 12) {
        slices_.resize(num_slices_ * 12);
        initBufferData();
    }

    auto [min_dist, max_dist, max_index] = sortVertices(view_dir);
    float cutoff = min_dist + (max_dist - min_dist) * front_;

    glm::vec3 vec_start[12];
    glm::vec3 vec_dir[12];
    float lambda[12];
    float lambda_inc[12];
    float denom;

    float plane_dist = min_dist;
    float plane_dist_inc = (max_dist - min_dist) / float(num_slices_);

    for (int e = 0; e < 12; e++) {
        auto edge = egs_[pts_[max_index][e]];

        vec_start[e] = vts_[edge[0]];
        vec_dir[e] = vts_[edge[1]] - vec_start[e];

        denom = glm::dot(vec_dir[e], view_dir);
        if (denom != 0.0) {
            lambda_inc[e] =  plane_dist_inc / denom;
            lambda[e]     = (plane_dist - glm::dot(vec_start[e], view_dir)) / denom;
        } else {
            lambda[e]     = -1.0f;
            lambda_inc[e] =  0.0f;
        }
    }

    glm::vec3 intersection[6];
    float dl[12];

    int count = 0;
    for (int i = num_slices_ - 1; i >= 0; --i) {
        if (!inverted && i * plane_dist_inc + min_dist < cutoff) continue;
        else if (inverted && max_dist - i * plane_dist_inc < cutoff) continue;

        for (int e = 0; e < 12; e++) dl[e] = lambda[e] + i * lambda_inc[e];

        if  (dl[0] >= 0.0 && dl[0] < 1.0)	{
            intersection[0] = vec_start[0] + dl[0] * vec_dir[0];
        } else if (dl[1] >= 0.0 && dl[1] < 1.0)	{
            intersection[0] = vec_start[1] + dl[1] * vec_dir[1];
        } else if (dl[3] >= 0.0 && dl[3] < 1.0)	{
            intersection[0] = vec_start[3] + dl[3] * vec_dir[3];
        } else {
            continue;
        }

        if (dl[2] >= 0.0 && dl[2] < 1.0){
            intersection[1] = vec_start[2] + dl[2] * vec_dir[2];
        } else {
            intersection[1] = intersection[0];
        }

        if  (dl[4] >= 0.0 && dl[4] < 1.0){
            intersection[2] = vec_start[4] + dl[4] * vec_dir[4];
        } else if (dl[5] >= 0.0 && dl[5] < 1.0){
            intersection[2] = vec_start[5] + dl[5] * vec_dir[5];
        } else {
            intersection[2] = vec_start[7] + dl[7] * vec_dir[7];
        }

        if	(dl[6] >= 0.0 && dl[6] < 1.0){
            intersection[3] = vec_start[6] + dl[6] * vec_dir[6];
        } else {
            intersection[3] = intersection[2];
        }

        if	(dl[8] >= 0.0 && dl[8] < 1.0){
            intersection[4] = vec_start[8] + dl[8] * vec_dir[8];
        } else if (dl[9] >= 0.0 && dl[9] < 1.0){
            intersection[4] = vec_start[9] + dl[9] * vec_dir[9];
        } else {
            intersection[4] = vec_start[11] + dl[11] * vec_dir[11];
        }

        if (dl[10] >= 0.0 && dl[10] < 1.0){
            intersection[5] = vec_start[10] + dl[10] * vec_dir[10];
        } else {
            intersection[5] = intersection[4];
        }

        for (auto id : ids_) slices_[count++] = intersection[id];
    }
}

void VolumeSlicer::draw() {
    glEnable(GL_BLEND);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(slices_.size()));

    glDisable(GL_BLEND);
}

void VolumeSlicer::drawOnScreen() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadow_.eye_texture_);

    glBindVertexArray(shadow_.vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
}

void VolumeSlicer::drawOnBuffer(ShaderProgram* vslice_shader,
                                ShaderProgram* vlight_shader,
                                bool is_view_inverted) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    shadow_.bind();

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);

    glBindVertexArray(vao_);
    for (int i = num_slices_; i >= 0; --i) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadow_.light_texture_);
        vslice_shader->use();
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        if(is_view_inverted) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
        }
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        vlight_shader->use();
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);
    }

    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VolumeSlicer::initBufferData() {
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(glm::vec3) * slices_.size(),
                 0,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

std::tuple<float, float, int> VolumeSlicer::sortVertices(const glm::vec3& view_dir) {
    float max_dist = 0;
    float min_dist = max_dist;
    int max_index = 0;
    for (size_t i = 0; i < sizeof(vts_) / sizeof(glm::vec3); i++) {
        float dist = glm::dot(view_dir, vts_[i]);

        if(dist > max_dist) {
            max_dist = dist;
            max_index = i;
        }

        if (dist < min_dist) min_dist = dist;
    }

    static constexpr float EPSILON = 0.0001f;
    max_dist += EPSILON;
    min_dist -= EPSILON;

    return {min_dist, max_dist, max_index};
}

void VolumeSlicer::setFront(float front) {
    if (front > front_) {
        for (size_t i = 0; i < slices_.size(); ++i) slices_[i] = glm::vec3{};
    }
    front_ = front;
}

} // namespace recastx::gui