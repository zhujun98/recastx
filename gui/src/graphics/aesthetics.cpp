#include "graphics/aesthetics.hpp"

namespace recastx::gui {

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

Colormap::Colormap() : idx_(*Colormap::options().begin()) {
    updateTexture();
}

Colormap::~Colormap() = default;

void Colormap::bind() const { texture_.bind(); }

void Colormap::unbind() const { texture_.unbind(); }

void Colormap::updateTexture() {
    auto& cmd = Colormap::data();
    int samples = cmd.TableSizes[idx_];
    std::vector<unsigned char> data;
    int offset = cmd.TableOffsets[idx_];
    for (int i = offset; i < offset + samples; ++i) {
        ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(cmd.Tables[i]);
        data.push_back(static_cast<unsigned char>(255 * rgb.x));
        data.push_back(static_cast<unsigned char>(255 * rgb.y));
        data.push_back(static_cast<unsigned char>(255 * rgb.z));
    }
    texture_.setData(data, samples);
}

const std::set<ImPlotColormap>& Colormap::options() {
    return options_;
}

const ImPlotColormapData& Colormap::data() {
    ImPlotContext& gp = *(ImPlot::GetCurrentContext());
    return gp.ColormapData;
}

void Colormap::set(ImPlotColormap idx) {
    if (idx_ != idx) {
        idx_ = idx;
        updateTexture();
    }
}

} // namespace recastx::gui