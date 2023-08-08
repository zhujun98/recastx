/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_WIREFEAME_H
#define GUI_WIREFEAME_H

#include <memory>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

namespace recastx::gui {

class ShaderProgram;

class Wireframe {

    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;
    std::unique_ptr<ShaderProgram> shader_;

    glm::vec4 color_;
    float lw_;

  public:

    explicit Wireframe(const glm::vec4& color = glm::vec4(1.f, 1.f, 1.f, 0.2f),
                       float line_width = 3.f);
    ~Wireframe();

    void render(const glm::mat4& view, const glm::mat4& projection);

    ShaderProgram* shader() { return shader_.get(); }
};

} // namespace recastx::gui

#endif //GUI_WIREFEAME_H
