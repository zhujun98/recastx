#ifndef GUI_COLORMAP_WIDGET_H
#define GUI_COLORMAP_WIDGET_H

#include "graphics/graph_node.hpp"
#include "graphics/aesthetics.hpp"

namespace tomcat::gui {

class ColormapWidget : public GraphNode {

    ImPlotColormap curr_;

    Colormap cm_;

public:

    ColormapWidget();

    ~ColormapWidget() override;

    void renderIm() override;

    [[nodiscard]] const Colormap& colormap() const;
};

} // namespace tomcat::gui

#endif //GUI_COLORMAP_WIDGET_H