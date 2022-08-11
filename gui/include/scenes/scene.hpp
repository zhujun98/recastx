#ifndef GUI_SCENES_SCENE_H
#define GUI_SCENES_SCENE_H

#include <map>
#include <memory>
#include <vector>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

#include "graphics/object_component.hpp"
#include "packet_publisher.hpp"
#include "ticker.hpp"

namespace gui {

class ShaderProgram;
class SceneCamera;

class Scene : public InputHandler, public PacketPublisher, public Ticker {
  protected:

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<SceneCamera> camera_;
    float pixel_size_ = 1.0;

    // FIXME: map of components
    std::map<std::string, std::unique_ptr<ObjectComponent>> components_;

  public:

    Scene();
    virtual ~Scene();

    virtual void draw(glm::mat4 window_matrix) = 0;

    void add_component(std::unique_ptr<ObjectComponent> component) {
        components_.insert(std::make_pair(component->identifier(), std::move(component)));
    }

    ObjectComponent& get_component(const std::string& identifier) {
        return *components_[identifier].get();
    }

    bool handleMouseButton(int button, int action) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, bool down, int mods) override;
    void tick(float time_elapsed) override;
    void describe();

    [[nodiscard]] SceneCamera& camera();
};

}  // namespace gui

#endif // GUI_SCENES_SCENE_H