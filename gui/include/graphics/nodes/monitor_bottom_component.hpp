#ifndef GUI_MONITOR_BOTTOM_COMPONENT_H
#define GUI_MONITOR_BOTTOM_COMPONENT_H

#include "./scene_component.hpp"

namespace tomcat::gui {

class Scene;

class MonitorBottomComponent : public DynamicSceneComponent {

    bool visible_ = true;

public:

    explicit MonitorBottomComponent(Scene& scene);

    ~MonitorBottomComponent() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void renderGl() override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_MONITOR_BOTTOM_COMPONENT_H
