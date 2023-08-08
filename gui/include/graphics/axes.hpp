/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_AXES_H
#define GUI_AXES_H

#include <memory>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

namespace recastx::gui {

class ShaderProgram;

class Axes {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

  public:

    Axes();
    ~Axes() = default;

    void render(const glm::mat4& view, const glm::mat4& projection, float scale);
};

} // namespace recastx::gui

#endif //GUI_AXES_H
