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

class ColormapSelector {

    static const std::set<ImPlotColormap> options_;

    static ImPlotColormap map_;

public:

    explicit ColormapSelector(const char* label);
    ~ColormapSelector();
};


class ColormapOld {

public:

    using ColormapGradientType = std::array<std::vector<std::pair<double, double>>, 3>;

    [[nodiscard]] static std::map<std::string, ColormapGradientType>& gradients() {
        static std::map<std::string, ColormapGradientType> cms = {
                {"bone", {{
                                  {{0.0, 0.0}, {0.746032, 0.652778}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {0.365079, 0.319444}, {0.746032, 0.777778}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {0.365079, 0.444444}, {1.0, 1.0}}
                          }}},
                {"gray", {{
                                  {{0.0, 0.0}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {1.0, 1.0}}
                          }}},
                {"hot", {{
                                  {{0.0, 0.416}, {0.36, 1.0}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {0.365079, 0.0}, {0.746032, 1.0}, {1.0, 1.0}},
                                  {{0.0, 0.0}, {0.74, 0.0}, {1.0, 1.0}}
                          }}}
        };
        return cms;
    };

private:

    ColormapTexture<> texture_;
    std::string name_;

public:

    ColormapOld();
    ~ColormapOld();

    void setColormap(const std::string& name);

    void describe();

    void bind();
    void unbind();
};

} // namespace tomcat::gui


#endif //GUI_AESTHETICS_H
