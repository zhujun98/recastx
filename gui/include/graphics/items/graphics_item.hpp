#ifndef GUI_SCENE_ITEM_H
#define GUI_SCENE_ITEM_H

#include <string>

#include <imgui.h>
#include <glm/glm.hpp>

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "ticker.hpp"
#include "tomcat/tomcat.hpp"

namespace tomcat::gui {

class Scene;

class GraphicsItem : public GraphGlNode, public InputHandler {

public:

    enum class ComponentType {
        STATIC,
        DYNAMIC
    };

protected:

    ComponentType type_;

    Scene& scene_;

    ImVec2 pos_;
    ImVec2 size_;

public:

    GraphicsItem(ComponentType type, Scene& scene);

    ~GraphicsItem() override;

    virtual void init();

    virtual void onWindowSizeChanged(int width, int height);

    [[nodiscard]] ComponentType type() const { return type_; }
};


class StaticGraphicsItem : public GraphicsItem {

public:

    explicit StaticGraphicsItem(Scene& scene);

    ~StaticGraphicsItem() override;

};


class DynamicGraphicsItem : public GraphicsItem {

public:

    explicit DynamicGraphicsItem(Scene& scene);

    ~DynamicGraphicsItem() override;

    virtual bool consume(const PacketDataEvent& data) = 0;
};

} // tomcat::gui

#endif // GUI_SCENE_ITEM_H
