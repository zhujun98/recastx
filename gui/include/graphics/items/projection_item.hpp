#ifndef GUI_PROJECTIONITEM_HPP
#define GUI_PROJECTIONITEM_HPP

#include "graphics/items/graphics_item.hpp"

namespace tomcat::gui {

class ProjectionItem : public GraphicsDataItem {

    int downsampling_factor_ = 1;

    float x_offset_ = 0.f;
    float y_offset_ = 0.f;

public:

    explicit ProjectionItem(Scene& scene);

    ~ProjectionItem() override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;

    bool consume(const ReconDataPacket& packet) override;
};

} // tomcat::gui

#endif //GUI_PROJECTIONITEM_HPP