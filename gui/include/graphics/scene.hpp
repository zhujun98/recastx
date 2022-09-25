#ifndef GUI_SCENE_H
#define GUI_SCENE_H

#include <memory>
#include <vector>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

#include "graphics/items/graphics_item.hpp"
#include "graphics/graph_node.hpp"
#include "graphics/viewport.hpp"
#include "ticker.hpp"
#include "client.hpp"

namespace tomcat::gui {

class ShaderProgram;
class Camera;

class Scene : public GraphNode, public InputHandler, public Ticker {

protected:

    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<Camera> camera_;
    Client* client_;

    std::vector<GraphicsItem*> items_;
    std::vector<GraphicsDataItem*> data_items_;

    ImVec2 pos_;
    ImVec2 size_;

    bool fixed_camera_ = false;

  public:

    explicit Scene(Client* client);

    ~Scene() override;

    virtual void onFrameBufferSizeChanged(int width, int height) = 0;

    void onWindowSizeChanged(int width, int height);

    virtual void render() = 0;

    void init();

    void addItem(GraphicsItem* item);

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(float offset) override;

    bool handleMouseMoved(float x, float y) override;

    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    template <typename T>
    void send(T&& packet) const {
        client_->send(std::forward<T>(packet));
    }
};

}  // namespace tomcat::gui

#endif // GUI_SCENES_SCENE_H