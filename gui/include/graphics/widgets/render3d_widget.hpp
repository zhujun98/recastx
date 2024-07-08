/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RENDER3DWIDGET_h
#define GUI_RENDER3DWIDGET_h

namespace recastx::gui {

class Volume;

class Render3DWidget {

    float volume_front_ = 0.f;
    const float volume_front_step_ = 0.005f;
    float iso_value = 0.f;

    Volume* volume_;

  public:

    explicit Render3DWidget(Volume* volume);

    void render();
};

} // namespace recastx::gui

#endif // GUI_RENDER3DWIDGET_h