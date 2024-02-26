/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef RECASTX_PREPROCITEM_HPP
#define RECASTX_PREPROCITEM_HPP

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class PreprocItem : public GraphicsItem {

    int downsampling_col_ = 1;
    int downsampling_row_ = 1;

    int32_t offsets_[2] = {0, 0};

    std::string ramp_filter_name_;

    bool setDownsampling();

    bool setOffset();

    bool setRampFilter();

  public:

    explicit PreprocItem(Scene& scene);

    ~PreprocItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    bool updateServerParams() override;
};

} // namespace recastx::gui

#endif //RECASTX_PREPROCITEM_HPP
