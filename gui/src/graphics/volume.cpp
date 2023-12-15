/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/volume.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

void VolumeSlicer::update(const glm::vec3& view_dir) {
    auto [min_dist, max_dist, max_index] = sortVertices(view_dir);

    glm::vec3 vec_start[12];
    glm::vec3 vec_dir[12];
    float lambda[12];
    float lambda_inc[12];
    float denom;

    float plane_dist = min_dist;
    float plane_dist_inc = (max_dist - min_dist) / float(K_NUM_SLICES);

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
    for (int i = K_NUM_SLICES - 1; i >= 0; i--) {
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


Volume::Volume() {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(VolumeSlicer::DataType::value_type) * slicer_.slices().size(),
                 0,
                 GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    auto vert =
#include "shaders/recon_volume.vert"
    ;
    auto frag =
#include "shaders/recon_volume.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);
};

Volume::~Volume() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Volume::setData(DataType&& data, const ShapeType& shape) {
    data_ = std::move(data);
    shape_ = shape;
    update_texture_ = true;
}

void Volume::preRender() {
    if (update_texture_) {
        if (!data_.empty()) {
            texture_.setData(data_,
                             static_cast<int>(shape_[0]),
                             static_cast<int>(shape_[1]),
                             static_cast<int>(shape_[2]));
        }
        update_texture_ = false;
        updateMinMaxVal();
    }
}

void Volume::render(const glm::mat4& view,
                    const glm::mat4& projection,
                    float min_v,
                    float max_v,
                    float alpha) {
    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("volumeData", 2);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    shader_->setFloat("alpha", alpha);
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    auto view_dir = glm::vec3(-view[0][2], -view[1][2], -view[2][2]);
    slicer_.update(view_dir);

    texture_.bind();

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    auto count = static_cast<int>(slicer_.slices().size());
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    sizeof(VolumeSlicer::DataType::value_type) * count,
                    slicer_.slices().data());

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, count);

    texture_.unbind();
}

void Volume::updateMinMaxVal() {
    if (data_.empty()) {
        min_max_vals_.reset();
        return;
    }
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

} // namespace recastx::gui