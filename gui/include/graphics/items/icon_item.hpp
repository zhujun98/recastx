/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_ICONITEM_H
#define GUI_ICONITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

class GlyphRenderer;
class Scene;
class Viewport;

class IconItem : public GraphicsItem, public GraphicsGLItem {

    std::unique_ptr<Viewport> vp_;

    static constexpr int font_size_ = 72;
    std::unique_ptr<ShaderProgram> glyph_shader_;
    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    glm::vec3 color1_;
    glm::vec3 color2_;
    glm::mat4 view_;
    glm::mat4 translate_;

public:

    explicit IconItem(Scene& scene);

    ~IconItem() override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;
};

}  // namespace recastx::gui

#endif // GUI_ICONITEM_H