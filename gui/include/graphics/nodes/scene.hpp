#ifndef GUI_SCENE_H
#define GUI_SCENE_H

#include <memory>
#include <vector>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

#include "./scene_component.hpp"
#include "graphics/graph_node.hpp"
#include "ticker.hpp"
#include "client.hpp"

namespace tomcat::gui {

class ShaderProgram;
class Camera;

class Scene : public GraphGlNode, public InputHandler, public Ticker {

protected:

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<Camera> camera_;
    Client* client_;

    std::vector<std::shared_ptr<SceneComponent>> components_;
    std::vector<std::shared_ptr<StaticSceneComponent>> static_components_;
    std::vector<std::shared_ptr<DynamicSceneComponent>> dynamic_components_;

    int width_ = 0;
    int height_ = 0;

    glm::mat4 projection_;

  public:

    explicit Scene(Client* client);

    ~Scene() override;

    void onFrameBufferSizeChanged(int width, int height);

    void onWindowSizeChanged(int width, int height);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    void renderIm() override;

    void renderGl() override;

    void init();

    void addComponent(const std::shared_ptr<SceneComponent>& component);

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(double offset) override;

    bool handleMouseMoved(double x, double y) override;

    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    template <typename T>
    void send(T&& packet) {
        client_->send(std::forward<T>(packet));
    }

    [[nodiscard]] Camera& camera() { return *camera_; }

    [[nodiscard]] const glm::mat4& projection() const { return projection_; }
};

}  // namespace tomcat::gui

#endif // GUI_SCENES_SCENE_H