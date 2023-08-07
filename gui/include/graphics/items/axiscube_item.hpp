/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_AXISCUBEITEM_H
#define GUI_AXISCUBEITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

class GlyphRenderer;
class Scene;
class Viewport;

class AxiscubeItem : public GraphicsItem, public GraphicsGLItem {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    std::unique_ptr<ShaderProgram> glyph_shader_;
    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    std::unique_ptr<Viewport> vp_;

    glm::vec3 text_color_;
    glm::mat4 top_;

public:

    explicit AxiscubeItem(Scene& scene);

    ~AxiscubeItem() override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;
};

}  // namespace recastx::gui

#endif // GUI_AXISCUBEITEM_H