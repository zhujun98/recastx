/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SCENE_H
#define GUI_SCENE_H

#include <any>
#include <memory>
#include <unordered_map>
#include <vector>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "graphics/items/graphics_item.hpp"
#include "graphics/graph_node.hpp"
#include "rpc_client.hpp"

namespace recastx::gui {

class ShaderProgram;
class Camera;
class Viewport;

class Scene : public GraphNode, public InputHandler {

public:

    using SceneStatus = std::unordered_map<std::string, std::any>;

protected:

    std::unique_ptr<Viewport> vp_;

    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<Camera> camera_;
    RpcClient* client_;

    std::vector<GraphicsItem*> items_;
    std::vector<GraphicsGLItem*> gl_items_;
    std::vector<GraphicsDataItem*> data_items_;

    ImVec2 pos_;
    ImVec2 size_;

    SceneStatus scene_status_;

  public:

    explicit Scene(RpcClient* client);

    ~Scene() override;

    RpcClient* client() { return client_; }

    void useViewport();

    [[nodiscard]] const glm::mat4& projectionMatrix() const;

    [[nodiscard]] const glm::mat4& viewMatrix() const;

    [[nodiscard]] float cameraDistance() const;

    void onFramebufferSizeChanged(int width, int height);

    void onWindowSizeChanged(int width, int height);

    virtual void render() = 0;

    void addItem(GraphicsItem* item);

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(float offset) override;

    bool handleMouseMoved(float x, float y) override;

    bool handleKey(int key, int action, int mods) override;

    void consumeData();

    void setStatus(const std::string& key, const std::any& value) {
        scene_status_[key] = value;
    };

    const std::any& getStatus(const std::string& key) {
        return scene_status_.at(key);
    }
};

}  // namespace recastx::gui

#endif // GUI_SCENES_SCENE_H