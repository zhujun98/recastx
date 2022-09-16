#ifndef GUI_SCENE_COMPONENT_H
#define GUI_SCENE_COMPONENT_H

#include <string>

#include "glm/glm.hpp"

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "ticker.hpp"
#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class Scene;

class SceneComponent : public GraphGlNode, public InputHandler {

public:

    enum class ComponentType {
        STATIC,
        DYNAMIC
    };

protected:

    ComponentType type_;

    Scene& scene_;

public:

    SceneComponent(ComponentType type, Scene& scene);

    ~SceneComponent() override;

    virtual void init();

    [[nodiscard]] ComponentType type() const { return type_; }
};


class StaticSceneComponent : public SceneComponent {

public:

    explicit StaticSceneComponent(Scene& scene);

    ~StaticSceneComponent() override;

};


class DynamicSceneComponent : public SceneComponent {

public:

    explicit DynamicSceneComponent(Scene& scene);

    ~DynamicSceneComponent() override;

    virtual bool consume(const PacketDataEvent& data) = 0;
};

} // tomcat::gui

#endif // GUI_SCENE_COMPONENT_H
