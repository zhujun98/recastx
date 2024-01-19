/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_STATUSBARITEM_H
#define GUI_STATUSBARITEM_H

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class Scene;

class StatusbarItem : public GraphicsItem {

    ImVec2 pos_;
    ImVec2 size_;

public:

    explicit StatusbarItem(Scene& scene);

    ~StatusbarItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;
};

}

#endif //GUI_STATUSBARITEM_H
