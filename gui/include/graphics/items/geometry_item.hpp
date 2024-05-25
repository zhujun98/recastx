/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_GEOMETRYITEM_H
#define GUI_GEOMETRYITEM_H

#include <common/config.hpp>
#include <memory>

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class Scene;

class GeometryItem : public GraphicsItem, public GraphicsGLItem {

    int beam_shape_;

    int col_count_;
    int row_count_;

    int angle_count_;
    int angle_range_;

    int slice_size_;
    int volume_size_;

    int x_[2];
    int y_[2];
    int z_[2];

public:

    explicit GeometryItem(Scene& scene);

    ~GeometryItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;

    bool updateServerParams() override;
};

}  // namespace recastx::gui

#endif // GUI_GEOMETRYITEM_H