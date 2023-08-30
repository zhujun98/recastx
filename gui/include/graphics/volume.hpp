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

inline constexpr size_t K_NUM_SLICES = 128;

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

    std::tuple<float, float, int> sortVertices(const glm::vec3& view_dir);

  public:

    [[nodiscard]] const DataType& slices() const { return slices_; }

    void update(const glm::vec3& view_dir);
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
