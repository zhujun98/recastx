/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MATERIAL_H
#define GUI_MATERIAL_H

#include <glm/glm.hpp>


namespace recastx::gui {

    struct Material {
        glm::vec3 color;
        float shininess;
        float iso_value;
    };

} // namespace recastx::gui

#endif // GUI_MATERIAL_H