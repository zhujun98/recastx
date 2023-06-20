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
#include "graphics/viewport.hpp"
#include "ticker.hpp"
#include "rpc_client.hpp"

namespace recastx::gui {

class ShaderProgram;
class Camera;
class RpcClient;

class Scene : public GraphNode, public InputHandler, public Ticker {

public:

    using SceneStatus = std::unordered_map<std::string, std::any>;

protected:

    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<Camera> camera_;
    RpcClient* client_;

    std::vector<GraphicsItem*> items_;
    std::vector<GraphicsDataItem*> data_items_;

    ImVec2 pos_;
    ImVec2 size_;

    bool fixed_camera_ = false;

    SceneStatus scene_status_;

  public:

    explicit Scene();

    ~Scene() override;

    void setClient(RpcClient* client) { client_ = client; }

    RpcClient* client() { return client_; }

    virtual void onFrameBufferSizeChanged(int width, int height) = 0;

    void onWindowSizeChanged(int width, int height);

    virtual void render() = 0;

    virtual void init();

    void addItem(GraphicsItem* item);

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(float offset) override;

    bool handleMouseMoved(float x, float y) override;

    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    void setStatus(const std::string& key, const std::any& value) {
        scene_status_[key] = value;
    };

    const std::any& getStatus(const std::string& key) {
        return scene_status_.at(key);
    }
};

}  // namespace recastx::gui

#endif // GUI_SCENES_SCENE_H