#ifndef GUI_STATUSBAR_COMPONENT_H
#define GUI_STATUSBAR_COMPONENT_H

#include "./scene_component.hpp"

namespace tomcat::gui {

class Scene;

class StatusbarComponent : public DynamicSceneComponent {

public:

    explicit StatusbarComponent(Scene& scene);

    ~StatusbarComponent() override;

    void renderIm(int width, int height) override;

    void renderGl(const glm::mat4& world_to_screen) override;

    bool consume(const PacketDataEvent& data) override;
};

}

#endif //GUI_STATUSBAR_COMPONENT_H
