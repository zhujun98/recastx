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

// class ColormapOld

ColormapOld::ColormapOld() : name_("hot") {
    setColormap(name_);
}

ColormapOld::~ColormapOld() = default;

void ColormapOld::setColormap(const std::string& name) {
    name_ = name;

    constexpr int samples = 100;

    auto& gradient = ColormapOld::gradients().at(name);

    auto interpolate =
            [](double z, std::vector<std::pair<double, double>>& xys) -> double {
                for (int i = 1; i < (int)xys.size(); ++i) {
                    if (z > xys[i].first)
                        continue;

                    auto val = xys[i - 1].second +
                            ((z - xys[i - 1].first) / (xys[i].first - xys[i - 1].first)) *
                            (xys[i].second - xys[i - 1].second);

                    return val;
                }

                return 0.0f;
            };

    std::vector<unsigned char> data(samples * 3);
    for (int j = 0; j < samples; ++j) {
        for (int i = 0; i < 3; ++i) {
            double intensity = (double)j / samples;
            data[j * 3 + i] = static_cast<unsigned char>(255 * interpolate(intensity, gradient[i]));
        }
    }

    texture_.setData(data, samples);
}

void ColormapOld::describe() {
    const std::string prev_cm = name_;
    if (ImGui::BeginCombo("Colormap", name_.c_str())) {
        for (auto& gradient : ColormapOld::gradients()) {
            bool is_selected = (name_ == gradient.first);
            if (ImGui::Selectable(gradient.first.c_str(), is_selected))
                name_ = gradient.first;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (prev_cm != name_) setColormap(name_);
}

void ColormapOld::bind() { texture_.bind(); }
void ColormapOld::unbind() { texture_.unbind(); }

} // namespace tomcat::gui