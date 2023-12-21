/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SCENEITEM_H
#define GUI_SCENEITEM_H

#include <string>
#include <unordered_map>
#include <any>
#include <variant>

#include <imgui.h>
#include <glm/glm.hpp>

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "projection.pb.h"
#include "reconstruction.pb.h"

namespace recastx::gui {

class Camera;
class Viewport;
class Scene;

class GraphicsItem : public GraphNode, public InputHandler {

protected:

    Scene& scene_;

public:

    explicit GraphicsItem(Scene& scene);

    ~GraphicsItem() override;

    virtual void onWindowSizeChanged(int width, int height) = 0;

    virtual void renderIm() = 0;

    virtual bool updateServerParams() { return false; }

    [[nodiscard]] Scene& scene() const { return scene_; }
};


class GraphicsDataItem {

public:

    using DataType = std::variant<rpc::ReconData, rpc::ProjectionData>;
    virtual bool consume(const DataType& packet) = 0;
};

class GraphicsGLItem {

public:

    using RenderParams = std::unordered_map<std::string, std::any>;

public:

    virtual void onFramebufferSizeChanged(int width, int height) = 0;

    virtual void preRenderGl() {};

    virtual void renderGl() = 0;
};

} // namespace recastx::gui

#endif // GUI_SCENEITEM_H
