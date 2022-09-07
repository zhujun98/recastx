#include "graphics/aesthetics.hpp"

namespace tomcat::gui {

const std::set<ImPlotColormap> Colormap::options_ {
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Viridis), // 4
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Plasma), // 5
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Hot), // 6
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Cool), // 7
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Pink), // 8
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Jet), // 9
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Spectral), //14
    static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Greys), // 15

};

Colormap::Colormap() :
        map_(static_cast<ImPlotColormap>(ImPlotColormap_::ImPlotColormap_Viridis)) {
    updateTexture();
}

void Colormap::describe() {
    ImPlotContext& gp = *(ImPlot::GetCurrentContext());
    auto& cmd = gp.ColormapData;
    auto prev_map_ = map_;
    if (ImGui::BeginCombo("Colormap##ReconComponent", cmd.GetName(map_))) {
        for (auto idx : options_) {
            const char* name = cmd.GetName(idx);
            if (ImGui::Selectable(name, map_ == idx)) map_ = idx;
        }
        ImGui::EndCombo();
    }

    if (prev_map_ != map_) updateTexture();
}

Colormap::~Colormap() = default;

void Colormap::bind() { texture_.bind(); }

void Colormap::unbind() { texture_.unbind(); }

void Colormap::updateTexture() {
    ImPlotContext& gp = *(ImPlot::GetCurrentContext());
    auto& cmd = gp.ColormapData;

    int samples = cmd.TableSizes[map_];
    std::vector<unsigned char> data;
    int offset = cmd.TableOffsets[map_];
    for (int i = offset; i < offset + samples; ++i) {
        ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(cmd.Tables[i]);
        data.push_back(static_cast<unsigned char>(255 * rgb.x));
        data.push_back(static_cast<unsigned char>(255 * rgb.y));
        data.push_back(static_cast<unsigned char>(255 * rgb.z));
    }
    texture_.setData(data, samples);
}

} // namespace tomcat::gui