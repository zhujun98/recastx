/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_ISOSURFACEINTERFACE_H
#define GUI_ISOSURFACEINTERFACE_H

#include <vector>

namespace recastx::gui {

struct SurfaceVertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

using SurfaceVertices = std::vector<SurfaceVertex>;

} // namespace recastx::gui

#endif // GUI_ISOSURFACEINTERFACE_H