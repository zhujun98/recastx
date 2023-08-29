/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUME_H
#define GUI_VOLUME_H

#include <array>

#include "textures.hpp"
#include "utils.hpp"

namespace recastx::gui {

inline constexpr size_t K_NUM_SLICES = 32;

class VolumeSlicer {
  public:

    using DataType = std::array<glm::vec3, K_NUM_SLICES * 12>;

  private:

    DataType slices_;

    static constexpr int ids_[12] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5};

    static constexpr glm::vec3 vts_[8] = {
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f)
    };

    static constexpr int egs_[12][2]= {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}
    };

    static constexpr int pts_[8][12] = {
            { 0,1,5,6,   4,8,11,9,  3,7,2,10 }, { 0,4,3,11,  1,2,6,7,   5,9,8,10 },
            { 1,5,0,8,   2,3,7,4,   6,10,9,11}, { 7,11,10,8, 2,6,1,9,   3,0,4,5  },
            { 8,5,9,1,   11,10,7,6, 4,3,0,2  }, { 9,6,10,2,  8,11,4,7,  5,0,1,3  },
            { 9,8,5,4,   6,1,2,0,   10,7,11,3}, { 10,9,6,5,  7,2,3,1,   11,4,8,0 }
    };

    std::tuple<float, float, int> sortVertices(const glm::vec3& view_dir) {
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

  public:

    [[nodiscard]] const DataType& slices() const { return slices_; }

    void update(const glm::vec3& view_dir) {
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
};

class ShaderProgram;

class Volume {

  public:

    using DataType = std::vector<float>;
    using SizeType = std::array<size_t, 3>;

  private:

    SizeType size_;
    DataType data_;

    VolumeTexture<float> texture_;

    VolumeSlicer slicer_;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    std::array<float, 2> min_max_vals_ {0.f, 0.f};

    void updateMinMaxVal();

public:

    Volume();
    ~Volume();

    void setData(DataType&& data, const SizeType& size);

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v,
                float alpha);

    void bind() const;

    void unbind() const;

    [[nodiscard]] const std::array<float, 2>& minMaxVals() const;
};


} // namespace recastx::gui

#endif // GUI_VOLUME_H
