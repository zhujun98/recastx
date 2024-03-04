/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MATERIALITEM_H
#define GUI_MATERIALITEM_H

#include "graphics_item.hpp"
#include "graphics/material.hpp"

namespace recastx::gui {

class MaterialItem : public GraphicsItem {

    Material material_;

    float ambient_[3];
    float diffuse_[3];
    float specular_[3];

  public:

    explicit MaterialItem(Scene& scene);

    ~MaterialItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    [[nodiscard]] const Material& material() const { return material_; }
};

} // namespace recastx::gui

#endif // GUI_MATERIALITEM_H