/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_GEOMETRYITEM_H
#define GUI_GEOMETRYITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class Scene;

class GeometryItem : public GraphicsItem, public GraphicsGLItem {

public:

    explicit GeometryItem(Scene& scene);

    ~GeometryItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;
};

}  // namespace recastx::gui

#endif // GUI_GEOMETRYITEM_H