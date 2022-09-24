#ifndef GUI_MONITOR_BOTTOM_COMPONENT_H
#define GUI_MONITOR_BOTTOM_COMPONENT_H

#include "graphics/components/scene_component.hpp"

namespace tomcat::gui {

class Scene;

class StatusbarComponent : public DynamicSceneComponent {

    bool visible_ = true;

public:

    explicit StatusbarComponent(Scene& scene);

    ~StatusbarComponent() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void renderGl() override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_MONITOR_BOTTOM_COMPONENT_H
