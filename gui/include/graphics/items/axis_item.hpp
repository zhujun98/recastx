/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_AXISITEM_H
#define GUI_AXISITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

class Axes;
class GlyphRenderer;
class Scene;
class Viewport;

class AxisItem : public GraphicsItem, public GraphicsGLItem {

    // axis

    bool show_axis_ = true;
    std::unique_ptr<Axes> axes_;

    // axis cube

    glm::vec3 text_color_;
    glm::mat4 top_;
    std::unique_ptr<ShaderProgram> glyph_shader_;
    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    std::unique_ptr<Viewport> vp_;

public:

    explicit AxisItem(Scene& scene);

    ~AxisItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;

    [[nodiscard]] bool axisVisible() const { return show_axis_; }
    void setAxisVisible(bool visible) { show_axis_ = visible; }
};

}  // namespace recastx::gui

#endif // GUI_AXISITEM_H