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
#include "state.pb.h"

namespace recastx::gui {

class Camera;
class Viewport;
class Scene;

class GraphicsItem : public GraphNode, public InputHandler {

protected:

    Scene& scene_;

    StatePacket_State state_;

public:

    explicit GraphicsItem(Scene& scene);

    ~GraphicsItem() override;

    virtual void renderIm() {};

    virtual void init();

    virtual void onWindowSizeChanged(int width, int height);

    virtual void setState(StatePacket_State state) { state_ = state; }

    [[nodiscard]] Scene& scene() const { return scene_; }
};


class GraphicsDataItem {

public:

    virtual bool consume(const ReconDataPacket& packet) = 0;
};

class GraphicsGLItem {

public:

    using RenderParams = std::unordered_map<std::string, std::any>;

public:

    virtual void renderGl(const glm::mat4& view,
                          const glm::mat4& projection,
                          const RenderParams& params) = 0;
};

} // namespace recastx::gui

#endif // GUI_SCENEITEM_H
