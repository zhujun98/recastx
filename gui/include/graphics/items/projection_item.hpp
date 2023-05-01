#ifndef GUI_PROJECTIONITEM_HPP
#define GUI_PROJECTIONITEM_HPP

#include "graphics/items/graphics_item.hpp"

namespace recastx::gui {

class ProjectionItem : public GraphicsItem {

    int downsampling_col_ = 1;
    int downsampling_row_ = 1;

    float x_offset_ = 0.f;
    float y_offset_ = 0.f;

    void setImageProcParameter();

public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void renderIm() override;
};

} // namespace recastx::gui

#endif //GUI_PROJECTIONITEM_HPP