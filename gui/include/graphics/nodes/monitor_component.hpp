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

    void renderIm(int width, int height) override;

    void renderGl(const glm::mat4& world_to_screen) override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_STATUSBAR_COMPONENT_H
