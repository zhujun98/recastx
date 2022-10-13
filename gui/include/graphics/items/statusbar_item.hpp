#ifndef GUI_STATUSBAR_ITEM_H
#define GUI_STATUSBAR_ITEM_H

#include "graphics/items/graphics_item.hpp"

namespace tomcat::gui {

class Scene;

class StatusbarItem : public GraphicsDataItem {

    ImVec2 pos_;
    ImVec2 size_;

    bool visible_ = true;

public:

    explicit StatusbarItem(Scene& scene);

    ~StatusbarItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_STATUSBAR_ITEM_H
