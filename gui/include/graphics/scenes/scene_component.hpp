#ifndef GUI_SCENE_COMPONENT_H
#define GUI_SCENE_COMPONENT_H

#include <string>

#include "glm/glm.hpp"

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "ticker.hpp"

namespace tomcat::gui {

class SceneComponent : public GraphNode, public InputHandler, public Ticker {
  public:
    SceneComponent() = default;
    ~SceneComponent() override = default;

    virtual void describe() = 0;

    [[nodiscard]] virtual std::string identifier() const = 0;
};

} // tomcat::gui

#endif // GUI_SCENE_COMPONENT_H
