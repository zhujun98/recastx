#ifndef GUI_SCENEITEM_H
#define GUI_SCENEITEM_H

#include <string>
#include <unordered_map>
#include <any>

#include <imgui.h>
#include <glm/glm.hpp>

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "ticker.hpp"
#include "reconstruction.pb.h"

namespace tomcat::gui {

class Camera;
class Viewport;
class Scene;

class GraphicsItem : public GraphNode, public InputHandler {

public:

    using RenderParams = std::unordered_map<std::string, std::any>;

protected:

    Scene& scene_;

public:

    explicit GraphicsItem(Scene& scene);

    ~GraphicsItem() override;

    virtual void renderIm() = 0;

    virtual void renderGl(const glm::mat4& view,
                          const glm::mat4& projection,
                          const RenderParams& params) = 0;

    virtual void init();

    virtual void onWindowSizeChanged(int width, int height);

    [[nodiscard]] Scene& scene() const { return scene_; }
};


class GraphicsDataItem : public GraphicsItem {

public:

    explicit GraphicsDataItem(Scene& scene);

    ~GraphicsDataItem() override;

    virtual bool consume(const ReconDataPacket& packet) = 0;
};

} // tomcat::gui

#endif // GUI_SCENEITEM_H
