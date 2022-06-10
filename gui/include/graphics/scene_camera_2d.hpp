#pragma once

#include <glm/glm.hpp>
#include "graphics/scene_camera.hpp"

namespace gui {

class SceneCamera2d : public SceneCamera {
   public:
    SceneCamera2d();

    glm::mat4 matrix() override;

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(float x, float y) override;
    bool handleKey(int key, bool down, int mods) override;

    glm::vec3& position() override { return position_; }
    glm::vec3& look_at() override { return look_at_; }

   private:
    glm::vec3 position_;
    glm::vec3 look_at_;

    float angle_ = 0.0f;
    float scale_ = 0.5f;

    float prev_x_ = -1.1f;
    float prev_y_ = -1.1f;

    bool dragging_ = false;
};

}  // namespace gui
