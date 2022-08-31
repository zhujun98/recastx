#ifndef GUI_AESTHETICS_H
#define GUI_AESTHETICS_H

#include <set>

#include <implot.h>
#include <implot_internal.h>

namespace tomcat::gui {

class ColormapSelector {

    static const std::set<ImPlotColormap> options_;

    static ImPlotColormap map_;

public:

    explicit ColormapSelector(const char* label);
    ~ColormapSelector();
};

} // namespace tomcat::gui


#endif //GUI_AESTHETICS_H
