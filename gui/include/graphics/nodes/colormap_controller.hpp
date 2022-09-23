#ifndef GUI_COLORMAP_CONTROLLER_H
#define GUI_COLORMAP_CONTROLLER_H

#include "graphics/graph_node.hpp"
#include "graphics/aesthetics.hpp"

namespace tomcat::gui {

class ColormapController : public GraphNode {

    ImPlotColormap curr_;

    Colormap cm_;

public:

    ColormapController();

    ~ColormapController() override;

    void renderIm() override;

    [[nodiscard]] const Colormap& colormap() const;
};

} // namespace tomcat::gui

#endif //GUI_COLORMAP_CONTROLLER_H
