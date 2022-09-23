#ifndef GUI_MONITOR_RIGHT_COMPONENT_H
#define GUI_MONITOR_RIGHT_COMPONENT_H

#include "./scene_component.hpp"

namespace tomcat::gui {

class Scene;

class MonitorRightComponent : public DynamicSceneComponent {

    bool visible_ = true;

public:

    explicit MonitorRightComponent(Scene& scene);

    ~MonitorRightComponent() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void renderGl() override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_MONITOR_RIGHT_COMPONENT_H
