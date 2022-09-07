#ifndef GUI_AESTHETICS_H
#define GUI_AESTHETICS_H

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

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

    explicit Colormap();

    ~Colormap();

    void describe();

    void bind();

    void unbind();
};

} // namespace tomcat::gui


#endif //GUI_AESTHETICS_H
