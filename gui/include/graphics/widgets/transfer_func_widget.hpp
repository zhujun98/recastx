/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_TRANSFERFUNCWIDGET_h
#define GUI_TRANSFERFUNCWIDGET_h

#include "graphics/aesthetics.hpp"

namespace recastx::gui {

class TransferFuncWidget {
    Colormap& cm_;

    std::map<float, float> alpha_;
    Alphamap& am_;
    const size_t MAX_NUM_POINTS_ = 10;
    const ImColor FRAME_COLOR_{180, 180, 180, 255};
    const ImColor POINT_COLOR_{180, 180, 90, 255};
    const ImColor LINE_COLOR_{180, 180, 120, 255};
    const float LINE_WIDTH_ = 2.f;
    const ImColor CIRCLE_COLOR_{255, 0, 0, 255};

    void renderColormapSelector();

    void renderColorbar();

    void renderAlphaEditor();

  public:

    TransferFuncWidget(Colormap& colormap, Alphamap& alphamap);

    void render();
};

} // namespace recastx::gui

#endif // GUI_TRANSFERFUNCWIDGET_h