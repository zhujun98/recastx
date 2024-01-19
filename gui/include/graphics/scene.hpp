/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<ShaderProgram> program_;

    RpcClient* client_;

    std::vector<GraphicsItem*> items_;
    std::vector<GraphicsGLItem*> gl_items_;
    std::vector<GraphicsDataItem*> data_items_;

    ImVec2 pos_;
    ImVec2 size_;

    SceneStatus scene_status_;

    rpc::ServerState_State server_state_ = rpc::ServerState_State_UNKNOWN;

    rpc::ScanMode_Mode  scan_mode_ = rpc::ScanMode_Mode_CONTINUOUS;
    uint32_t scan_update_interval_;

    bool setScanMode();

    bool updateServerParams();

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

    bool consume(const RpcClient::DataType& data);

    void setStatus(const std::string& key, const std::any& value) {
        scene_status_[key] = value;
    };

    const std::any& getStatus(const std::string& key) {
        return scene_status_.at(key);
    }

    rpc::ServerState_State serverState() const { return server_state_; }

    void connectServer();

    void startAcquiring();

    void stopAcquiring();

    void startProcessing();

    void stopProcessing();
};

}  // namespace recastx::gui

#endif // GUI_SCENES_SCENE_H