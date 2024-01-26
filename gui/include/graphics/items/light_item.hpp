/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHTITEM_H
#define GUI_LIGHTITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/light.hpp"

namespace recastx::gui {

class Scene;

class LightItem : public GraphicsItem, public GraphicsGLItem {

    Light light_;

    glm::vec3 color_;
    float ambient_;
    float diffuse_;
    float specular_;

public:

    explicit LightItem(Scene& scene);

    ~LightItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void renderGl() override;

    [[nodiscard]] const Light& light() const { return light_; }
};

}  // namespace recastx::gui

#endif // GUI_LIGHTITEM_H