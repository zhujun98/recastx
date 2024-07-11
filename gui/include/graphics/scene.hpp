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
struct Light;
struct Material;

class AxisItem;
class AxisCubeItem;
class IconItem;
class GeometryItem;
class LightItem;
class LoggingItem;
class MaterialItem;
class PreprocItem;
class ProjectionItem;
class ReconItem;
class StatusbarItem;

class Scene : public GraphNode, public InputHandler {

public:

    using SceneStatus = std::unordered_map<std::string, std::any>;

protected:

    struct Layout {
        int w;
        int h;
        double sw;
        double sh;
        int mw;
        int mh;
        int lw;
        int rw;
        int th;
        int bh;
    };

    Layout layout_;

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<ShaderProgram> program_;

    RpcClient* client_;

    std::vector<GraphicsItem*> items_;
    std::vector<GraphicsGLItem*> gl_items_;
    std::vector<GraphicsDataItem*> data_items_;

    SceneStatus scene_status_;

    rpc::ServerState_State server_state_ = rpc::ServerState_State_UNKNOWN;

    rpc::ScanMode_Mode  scan_mode_ = rpc::ScanMode_Mode_STATIC;
    uint32_t scan_update_interval_;

    std::unique_ptr<AxisItem> axis_item_;
    std::unique_ptr<AxisCubeItem> axiscube_item_;
    std::unique_ptr<IconItem> icon_item_;
    std::unique_ptr<GeometryItem> geometry_item_;
    std::unique_ptr<LightItem> light_item_;
    std::unique_ptr<LoggingItem> logging_item_;
    std::unique_ptr<MaterialItem> material_item_;
    std::unique_ptr<PreprocItem> preproc_item_;
    std::unique_ptr<ProjectionItem> projection_item_;
    std::unique_ptr<ReconItem> recon_item_;
    std::unique_ptr<StatusbarItem> statusbar_item_;

    virtual void updateLayout(int width, int height);

    bool setScanMode();

    bool updateServerParams();

    void renderMenubarRight();
    void renderMainControl();
    void renderScanModeControl();
    void renderCameraControl();

public:

    explicit Scene(RpcClient* client);

    ~Scene() override;

    RpcClient* client() { return client_; }

    [[nodiscard]] const Layout& layout() const { return layout_; }

    [[nodiscard]] const glm::mat4& cameraMatrix() const;

    [[nodiscard]] glm::vec3 cameraDir() const;

    [[nodiscard]] glm::vec3 cameraPosition() const;

    [[nodiscard]] float cameraDistance() const;

    [[nodiscard]] const Light& light() const;

    [[nodiscard]] const Material& material() const;

    void onFramebufferSizeChanged(int width, int height);

    void onWindowSizeChanged(int width, int height);

    void render();

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