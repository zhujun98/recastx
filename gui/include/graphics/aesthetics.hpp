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

class Colormap : public Texture {

    static const std::set<ImPlotColormap> options_;

    ImPlotColormap idx_;

    void updateTexture();

  public:

    explicit Colormap();

    ~Colormap() override;

    static const std::set<ImPlotColormap>& options();

    static const ImPlotColormapData& data();

    [[nodiscard]] ImPlotColormap get() const { return idx_; }

    void set(ImPlotColormap idx);
};


class Alphamap : public Texture {

    std::vector<float> data_;

  public:

    Alphamap();

    ~Alphamap() override;

    void set(const std::map<float, float>& alphamap);
};

} // namespace recastx::gui


#endif //GUI_AESTHETICS_H
