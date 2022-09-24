#ifndef GUI_AESTHETICS_H
#define GUI_AESTHETICS_H

#include <map>
#include <set>

#include "GL/gl3w.h"

#include <implot.h>
#include <implot_internal.h>

#include "textures.hpp"

namespace tomcat::gui {

class Colormap {

    static const std::set<ImPlotColormap> options_;

    ImPlotColormap idx_;

    ColormapTexture<> texture_;

public:

    Colormap();

    ~Colormap();

    void updateTexture();

    void bind() const;

    void unbind() const;

    static const std::set<ImPlotColormap>& options();

    static const ImPlotColormapData& data();

    [[nodiscard]] ImPlotColormap get() const { return idx_; }

    void set(ImPlotColormap idx);
};

} // namespace tomcat::gui


#endif //GUI_AESTHETICS_H
