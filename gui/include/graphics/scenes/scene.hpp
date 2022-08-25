#ifndef GUI_SCENE_H
#define GUI_SCENE_H

#include <map>
#include <memory>
#include <vector>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

#include "scene_component.hpp"
#include "graphics/graph_node.hpp"
#include "ticker.hpp"
#include "server.hpp"

namespace tomcat::gui {

class ShaderProgram;
class SceneCamera;

class Scene : public GraphNode, public InputHandler, public Ticker {
  protected:

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<SceneCamera> camera_;
    Server* server_;

    float pixel_size_ = 1.0;

    // FIXME: map of components
    std::map<std::string, std::unique_ptr<SceneComponent>> components_;

  public:

    Scene();
    ~Scene() override;

    virtual void describe();

    void addComponent(std::unique_ptr<SceneComponent> component);

    SceneComponent& component(const std::string& identifier) {
        return *components_[identifier];
    }

    bool handleMouseButton(int button, int action) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    void setPublisher(Server* server);

    template <typename T>
    void send(T&& packet) {
        server_->send(std::forward<T>(packet));
    }

    [[nodiscard]] SceneCamera& camera();
};

}  // namespace tomcat::gui

#endif // GUI_SCENES_SCENE_H