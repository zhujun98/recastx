#pragma once

#include <vector>
#include <utility>
#include <string>

#include <glm/glm.hpp>
#include <GL/gl3w.h>

#include "input_handler.hpp"
#include "ticker.hpp"

namespace gui {

class SceneCamera : public InputHandler, public Ticker {

    int current_scheme_ = -1;
    std::vector<std::string> schemes_;
    GLuint colormap_texture_id_;

  protected:

    bool interaction_disabled_ = false;

  public:

    SceneCamera();
    virtual ~SceneCamera();

    virtual glm::mat4 matrix() = 0;

    virtual void set_look_at(glm::vec3 /* center */) {}
    void tick(float) override {}

    [[nodiscard]] GLuint colormap() const { return colormap_texture_id_; }
    void set_colormap(int scheme);

    virtual glm::vec3& position() = 0;
    virtual glm::vec3& look_at() = 0;
    virtual void reset_view() {};

    virtual void describe();

    void toggle_interaction() { interaction_disabled_ = ! interaction_disabled_; }
};

} // namespace gui
