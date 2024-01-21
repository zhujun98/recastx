/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
class Scene;

class AxisItem : public GraphicsItem, public GraphicsGLItem {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

public:

    explicit AxisItem(Scene& scene);

    ~AxisItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;
};

}  // namespace recastx::gui

#endif // GUI_AXISITEM_H