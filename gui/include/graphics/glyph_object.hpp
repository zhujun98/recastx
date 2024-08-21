/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_GLYPH_OBJECT_H
#define GUI_GLYPH_OBJECT_H

#include <string>

#include <glm/glm.hpp>

#include "renderable.hpp"

namespace recastx::gui {

class GlyphObject : public Renderable {

    static const char *vertex_shader;
    static const char *fragment_shader;

    GLuint VAO_;
    GLuint VBO_;
    GLuint EBO_;

    std::string text_;
    glm::vec3 color_;

    void init();

  public:

    explicit GlyphObject(std::string text);

    ~GlyphObject() override;

    void render(Renderer *renderer) override;

    void setColor(glm::vec3 rgb) { color_ = rgb; }
};

} // recastx;;gui

#endif //GUI_GLYPH_OBJECT_H

