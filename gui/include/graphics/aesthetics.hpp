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

    ImPlotColormap map_;

    ColormapTexture<> texture_;

    void updateTexture();

public:

    Colormap();

    ~Colormap();

    void describe();

    void bind();

    void unbind();
};

} // namespace tomcat::gui


#endif //GUI_AESTHETICS_H
