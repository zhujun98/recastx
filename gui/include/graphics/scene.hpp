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

class Scene : public GraphGlNode, public InputHandler, public Ticker {

protected:

    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<Camera> camera_;
    Client* client_;

    std::vector<std::unique_ptr<Viewport>> viewports_;
    std::vector<std::shared_ptr<GraphicsItem>> components_;
    std::vector<std::shared_ptr<StaticGraphicsItem>> static_components_;
    std::vector<std::shared_ptr<DynamicGraphicsItem>> dynamic_components_;

    ImVec2 pos_;
    ImVec2 size_;

  public:

    explicit Scene(Client* client);

    ~Scene() override;

    virtual void onFrameBufferSizeChanged(int width, int height) = 0;

    void onWindowSizeChanged(int width, int height);

    void renderIm() override;

    void renderGl() override;

    void init();

    void addComponent(const std::shared_ptr<GraphicsItem>& component);

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(float offset) override;

    bool handleMouseMoved(float x, float y) override;

    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    template <typename T>
    void send(T&& packet) {
        client_->send(std::forward<T>(packet));
    }

    [[nodiscard]] Camera& camera() { return *camera_; }

    [[nodiscard]] const glm::mat4& projection() const { return viewports_[0]->projection(); }
};

}  // namespace tomcat::gui

#endif // GUI_SCENES_SCENE_H