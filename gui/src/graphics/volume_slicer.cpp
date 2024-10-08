/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/volume_slicer.hpp"

namespace recastx::gui {

VolumeSlicer::VolumeSlicer(size_t num_slices) {
    resize(num_slices);
}

VolumeSlicer::~VolumeSlicer() = default;

bool VolumeSlicer::resize(size_t num_slices) {
    if (num_slices_ != num_slices) {
        num_slices_ = num_slices;
        slices_.resize(12 * num_slices);
        return true;
    }
    return false;
}

void VolumeSlicer::update(const glm::vec3& view_dir) {
    auto [min_dist, max_dist, max_index] = sortVertices(view_dir);

    glm::vec3 vec_start[12];
    glm::vec3 vec_dir[12];
    float lambda[12];
    float lambda_inc[12];

    float plane_dist = min_dist;
    float plane_dist_inc = (max_dist - min_dist) / float(num_slices_ - 1);
    float plane_dist_inc_frac = 1.f / float(num_slices_ - 1);


    const auto& path = paths_[max_index];
    for (int e = 0; e < 12; ++e) {
        auto edge = edges_[path[e]];

        vec_start[e] = vertices_[edge[0]];
        vec_dir[e] = vertices_[edge[1]] - vec_start[e];

        float norm = glm::dot(vec_dir[e], view_dir);

        if (norm != 0.0) {
            lambda_inc[e] =  plane_dist_inc / norm;
            lambda[e]     = (plane_dist - glm::dot(vec_start[e], view_dir)) / norm;
        } else {
            lambda_inc[e] =  0.0f;
            lambda[e]     = -1.0f;
        }
    }

    glm::vec3 intersection[6];
    float dl[12];

    int count = 0;
    for (int i = int(num_slices_) - 1; i >= 0; --i) {
        for (int e = 0; e < 12; e++) {
            dl[e] = lambda[e] + (float)i * lambda_inc[e];
        }

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

        for (auto idx : indices_) {
            slices_[count++] = { intersection[idx], plane_dist_inc_frac * (float)i };
        }
    }
}

std::tuple<float, float, size_t> VolumeSlicer::sortVertices(const glm::vec3& view_dir) {
    float max_dist = glm::dot(view_dir, vertices_[0]);;
    float min_dist = max_dist;
    size_t max_index = 0;
    for (size_t i = 1; i < sizeof(vertices_) / sizeof(glm::vec3); i++) {
        float dist = glm::dot(view_dir, vertices_[i]);

        if (dist > max_dist) {
            max_dist = dist;
            max_index = i;
        } else if (dist < min_dist) {
            min_dist = dist;
        }
    }

    const float epsilon = 0.0001f * float(max_dist - min_dist);
    max_dist -= epsilon;
    min_dist += epsilon;

    return { min_dist, max_dist, max_index };
}

} // namespace recastx::gui