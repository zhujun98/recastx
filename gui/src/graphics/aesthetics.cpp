/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
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
    glBindTexture(target_, texture_);
    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    updateTexture();
    initialized_ = true;
}

Colormap::~Colormap() = default;

void Colormap::updateTexture() {
    auto& cmd = Colormap::data();
    int size = cmd.TableSizes[idx_];
    assert(size >= 256);
    std::vector<unsigned char> data;
    int offset = cmd.TableOffsets[idx_];
    for (int i = offset; i < offset + size; ++i) {
        ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(cmd.Tables[i]);
        data.push_back(static_cast<unsigned char>(255 * rgb.x));
        data.push_back(static_cast<unsigned char>(255 * rgb.y));
        data.push_back(static_cast<unsigned char>(255 * rgb.z));
    }

    setData(data.data(), size, 3);
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


Alphamap::Alphamap() : data_(256) {
    set({{0.f, 0.f}, {1.f, 1.f}});
    initialized_ = true;
}

Alphamap::~Alphamap() = default;

void Alphamap::set(const std::map<float, float>& alphamap) {
    // TODO: improve
    std::vector<float> x;
    std::vector<float> alpha;
    for (auto [k, v] : alphamap) {
        x.push_back(k);
        alpha.push_back(v);
    }

    size_t j = 0;
    size_t n = data_.size();
    while (static_cast<float>(j) < x[0] * static_cast<float>(n) && j < n) data_[j++] = alpha[0];
    for (size_t i = 1; i < x.size(); ++i) {
        while (static_cast<float>(j) < x[i] * static_cast<float>(n)) {
            data_[j] = alpha[i - 1] + (alpha[i] - alpha[i - 1]) * (static_cast<float>(j) / n - x[i - 1]) / (x[i] - x[i - 1]);
            ++j;
        }
    }
    while (j < n) data_[j++] = alpha.back();

    setData(data_.data(), data_.size(), 1);
}

} // namespace recastx::gui