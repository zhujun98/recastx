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

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class ProjectionItem : public GraphicsItem, public GraphicsDataItem {

    ImVec2 pos_;
    ImVec2 size_;

    bool visible_ = true;

  public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    bool updateServerParams() override;

    bool consume(const DataType& packet) override;
};

} // namespace recastx::gui

#endif //GUI_PROJECTIONITEM_HPP