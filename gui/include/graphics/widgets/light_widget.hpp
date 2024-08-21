/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHTWIDGET_H
#define GUI_LIGHTWIDGET_H

#include <memory>
#include <string>

#include "widget.hpp"
#include "graphics/light.hpp"

namespace recastx::gui {

class LightWidget : public Widget {

    std::shared_ptr<Light> light_;

  public:

    explicit LightWidget(std::shared_ptr<Light> light);

    void draw() override;
};

} // namespace recastx::gui

#endif // GUI_LIGHTWIDGET_H
