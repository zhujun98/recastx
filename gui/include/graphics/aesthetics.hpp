/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_AESTHETICS_H
#define GUI_AESTHETICS_H

#include <map>
#include <set>

#include "GL/gl3w.h"

#include <implot.h>
#include <implot_internal.h>

#include "textures.hpp"

namespace recastx::gui {

class Texture1D : public Texture {

  protected:

    int x_;

    void configureTexture() {
        glBindTexture(target_, texture_);
        glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    template<typename T>
    void initTexture(T* data, int num_channels) {
        glBindTexture(target_, texture_);
        auto fmt = details::getTextureFormat<T>(num_channels);
        glTexImage1D(target_, 0, fmt.internal_format, x_, 0, fmt.format, fmt.type, data);
    }

  public:

    Texture1D() : Texture(GL_TEXTURE_1D), x_(0) {
        configureTexture();
    }

    template<typename T>
    void setData(const T* data, int x, int num_channels) {
        if (x != x_) {
            x_ = x;
            initTexture(data, num_channels);
        } else {
            glBindTexture(target_, texture_);
            auto fmt = details::getTextureFormat<T>(num_channels);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, x, fmt.format, fmt.type, data);
        }

        initialized_ = true;
    }
};

class Colormap : public Texture1D {

    static const std::set<ImPlotColormap> options_;

    ImPlotColormap idx_;

    void updateTexture();

  public:

    explicit Colormap();

    ~Colormap() override;

    static const std::set<ImPlotColormap>& options();

    static const ImPlotColormapData& data();

    [[nodiscard]] ImPlotColormap get() const { return idx_; }

    void set(ImPlotColormap idx);
};


class Alphamap : public Texture1D {

    std::vector<float> data_;

  public:

    Alphamap();

    ~Alphamap() override;

    void set(const std::map<float, float>& alphamap);
};

} // namespace recastx::gui


#endif //GUI_AESTHETICS_H
