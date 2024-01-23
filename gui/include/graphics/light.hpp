/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHT_H
#define GUI_LIGHT_H

#include <glm/glm.hpp>


namespace recastx::gui {

struct Light {
    bool is_enabled;
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

} // namespace recastx::gui

#endif // GUI_LIGHT_H