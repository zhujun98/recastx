#include "graphics/aesthetics.hpp"

namespace tomcat::gui {

const std::set<ImPlotColormap> ColormapSelector::options_ {
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Spectral),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Viridis),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Greys),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Plasma),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Hot),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Cool),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Pink),
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Jet)
};

ImPlotColormap ColormapSelector::map_ = static_cast<ImPlotColormap>(
        ImPlotColormap_::ImPlotColormap_Spectral);

ColormapSelector::ColormapSelector(const char* label) {
    ImPlotContext& gp = *(ImPlot::GetCurrentContext());
    if (ImGui::BeginCombo(label, gp.ColormapData.GetName(ColormapSelector::map_))) {
        for (auto idx : options_) {
            const char* name = gp.ColormapData.GetName(idx);
            if (ImGui::Selectable(name, ColormapSelector::map_ == idx)) {
                ColormapSelector::map_ = idx;
            }
        }
        ImGui::EndCombo();
    }

    ImPlot::PushColormap(ColormapSelector::map_);
}

ColormapSelector::~ColormapSelector() {
    ImPlot::PopColormap();
}

} // namespace tomcat::gui