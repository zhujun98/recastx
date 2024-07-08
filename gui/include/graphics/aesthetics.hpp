/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_AESTHETICS_H
#define GUI_AESTHETICS_H

#include <map>
#include <set>

#include "GL/gl3w.h"

#include <implot.h>
#include <implot_internal.h>

#include "textures.hpp"

namespace recastx::gui {

class Colormap {

    static const std::set<ImPlotColormap> options_;

    ImPlotColormap idx_;

    ColormapTexture texture_;

    void updateTexture();

  public:

    Colormap();

    static const std::set<ImPlotColormap>& options();

    static const ImPlotColormapData& data();

    [[nodiscard]] ImPlotColormap get() const { return idx_; }

    void set(ImPlotColormap idx);

    void bind() const { texture_.bind(); }
    void unbind() const { texture_.unbind(); }
};

class Alphamap {

    std::vector<float> data_;

    AlphamapTexture texture_;

  public:

    Alphamap();

    void set(const std::map<float, float>& alphamap);

    void bind() const { texture_.bind(); }
    void unbind() const { texture_.unbind(); }
};

} // namespace recastx::gui


#endif //GUI_AESTHETICS_H
