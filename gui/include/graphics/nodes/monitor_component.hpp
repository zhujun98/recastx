#ifndef GUI_STATUSBAR_COMPONENT_H
#define GUI_STATUSBAR_COMPONENT_H

#include "./scene_component.hpp"

namespace tomcat::gui {

class Scene;

class MonitorComponent : public DynamicSceneComponent {

    bool visible_ = true;

public:

    explicit MonitorComponent(Scene& scene);

    ~MonitorComponent() override;

    void renderIm() override;

    void renderGl() override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_STATUSBAR_COMPONENT_H
