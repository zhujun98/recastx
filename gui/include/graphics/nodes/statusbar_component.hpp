#ifndef GUI_STATUSBAR_COMPONENT_H
#define GUI_STATUSBAR_COMPONENT_H

#include "scene_component.hpp"

namespace tomcat::gui {

class Scene;

class StatusbarComponent : public SceneComponent {

    Scene& scene_;

public:

    explicit StatusbarComponent(Scene& scene);

    ~StatusbarComponent();

};

}

#endif //GUI_STATUSBAR_COMPONENT_H
