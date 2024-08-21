/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VERTEX_H
#define GUI_VERTEX_H

#include <glm/glm.hpp>

namespace recastx::gui {

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
    glm::vec3 normal;
};

}

#endif // GUI_VERTEX_H