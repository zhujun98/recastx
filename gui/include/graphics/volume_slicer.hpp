/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUMESLICER_H
#define GUI_VOLUMESLICER_H

#include <iostream>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>

#include "graphics/style.hpp"

namespace recastx::gui {

class VolumeSlicer {

  public:

    using SliceVertex = glm::vec4;

  protected:

    size_t num_slices_ = 0;
    std::vector<SliceVertex> slices_;

    static constexpr int indices_[12] = {
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 5
    };

    static constexpr glm::vec3 vertices_[8] = {
            glm::vec3(-0.5, -0.5, -0.5),
            glm::vec3( 0.5, -0.5, -0.5),
            glm::vec3( 0.5,  0.5, -0.5),
            glm::vec3(-0.5,  0.5, -0.5),
            glm::vec3(-0.5, -0.5,  0.5),
            glm::vec3( 0.5, -0.5,  0.5),
            glm::vec3( 0.5,  0.5,  0.5),
            glm::vec3(-0.5,  0.5,  0.5)
    };

    static constexpr int edges_[12][2]= {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
            {4, 5}, {5, 6}, {6, 7}, {7, 4}
    };

    static constexpr int paths_[8][12] = {
            { 0,1,5,6,   4,8,11,9,  3,7,2,10 },
            { 0,4,3,11,  1,2,6,7,   5,9,8,10 },
            { 1,5,0,8,   2,3,7,4,   6,10,9,11},
            { 7,11,10,8, 2,6,1,9,   3,0,4,5  },
            { 8,5,9,1,   11,10,7,6, 4,3,0,2  },
            { 9,6,10,2,  8,11,4,7,  5,0,1,3  },
            { 9,8,5,4,   6,1,2,0,   10,7,11,3},
            { 10,9,6,5,  7,2,3,1,   11,4,8,0 }
    };

    bool resize(size_t num_slices);

    std::tuple<float, float, size_t> sortVertices(const glm::vec3& view_dir);

  public:

    explicit VolumeSlicer(size_t num_slices);

    virtual ~VolumeSlicer();

    void update(const glm::vec3& view_dir);
};

} // namespace recastx::gui

#endif // GUI_VOLUMESLICER_H