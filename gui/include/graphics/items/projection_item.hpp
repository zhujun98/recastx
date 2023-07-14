/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTIONITEM_HPP
#define GUI_PROJECTIONITEM_HPP

#include <map>
#include <string>

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class ProjectionItem : public GraphicsItem {

  public:

    inline static const std::map<std::string, std::string> filter_options {
            {"shepp", "Shepp-Logan"},
            {"ramlak", "Ram-Lak"}};

  private:

    int downsampling_col_ = 1;
    int downsampling_row_ = 1;

    float x_offset_ = 0.f;
    float y_offset_ = 0.f;

    std::string ramp_filter_name_;

    bool setDownsampling();

    bool setRampFilter();

  public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void renderIm() override;

    bool updateServerParams() override;
};

} // namespace recastx::gui

#endif //GUI_PROJECTIONITEM_HPP