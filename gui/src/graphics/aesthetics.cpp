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

ColormapOld::ColormapOld() : curr_cm_("hot") {
    glGenTextures(1, &cm_texture_id_);
    setColormap(curr_cm_);
}

ColormapOld::~ColormapOld() = default;

void ColormapOld::setColormap(const std::string& name) {
    constexpr int samples = 100;

    auto& gradient = ColormapOld::gradients().at(name);

    curr_cm_ = name;
    auto interpolate =
            [](double z, std::vector<std::pair<double, double>>& xys) -> double {
                for (int i = 1; i < (int)xys.size(); ++i) {
                    if (z > xys[i].first)
                        continue;

                    auto val =
                            xys[i - 1].second +
                            ((z - xys[i - 1].first) / (xys[i].first - xys[i - 1].first)) *
                            (xys[i].second - xys[i - 1].second);

                    return val;
                }

                return 0.0f;
            };

    unsigned char image[samples * 3];
    for (int j = 0; j < samples; ++j) {
        for (int i = 0; i < 3; ++i) {
            double intensity = (double)j / samples;
            image[j * 3 + i] = (unsigned char)(255 * interpolate(intensity, gradient[i]));
        }
    }

    glBindTexture(GL_TEXTURE_1D, cm_texture_id_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, samples, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, 0);
}

void ColormapOld::describe() {
    const std::string prev_cm = curr_cm_;
    if (ImGui::BeginCombo("Colormap", curr_cm_.c_str())) {
        for (auto& gradient : ColormapOld::gradients()) {
            bool is_selected = (curr_cm_ == gradient.first);
            if (ImGui::Selectable(gradient.first.c_str(), is_selected))
                curr_cm_ = gradient.first;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (prev_cm != curr_cm_) setColormap(curr_cm_);
}

void ColormapOld::bind() const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, cm_texture_id_);
}

void ColormapOld::unbind() const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, 0);
}

} // namespace tomcat::gui