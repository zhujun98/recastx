#pragma once

#include <map>
#include <memory>
#include <vector>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "object_component.hpp"
#include "packet_listener.hpp"
#include "ticker.hpp"

namespace gui {

class ShaderProgram;
class SceneCamera;

class SceneObject : public InputHandler, public PacketPublisher, public Ticker {
   public:
    SceneObject();
    virtual ~SceneObject();

    virtual void draw(glm::mat4 window_matrix) = 0;

    virtual SceneCamera& camera() { return *camera_; }

    virtual void set_data(std::vector<unsigned char>& /* data */, int /* slice */) {}

    void add_component(std::unique_ptr<ObjectComponent> component) {
        components_.insert(
            std::make_pair(component->identifier(), std::move(component)));
    }

    ObjectComponent& get_component(const std::string& identifier) {
        return *components_[identifier].get();
    }

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, bool down, int mods) override;
    void tick(float time_elapsed) override;
    void describe();

  protected:

    virtual void update_image_(int /* slice */) {}

    GLuint vao_handle_;
    GLuint vbo_handle_;
    std::unique_ptr<ShaderProgram> program_;
    std::unique_ptr<SceneCamera> camera_;
    float pixel_size_ = 1.0;

    // FIXME: map of components
    std::map<std::string, std::unique_ptr<ObjectComponent>> components_;
};

}  // namespace gui
