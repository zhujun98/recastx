#ifndef GUI_GRAPHICS_SCENE_CAMERA2D_H
#define GUI_GRAPHICS_SCENE_CAMERA2D_H

#include <glm/glm.hpp>
#include "graphics/scene_camera.hpp"

namespace gui {

class SceneCamera2d : public SceneCamera {

    glm::vec3 position_;
    glm::vec3 look_at_;

    float angle_ = 0.0f;
    float scale_ = 0.5f;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;

    bool dragging_ = false;

  public:

    SceneCamera2d();

    glm::mat4 matrix() override;

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, bool down, int mods) override;

    glm::vec3& position() override { return position_; }
    glm::vec3& look_at() override { return look_at_; }
};

}  // namespace gui

#endif // GUI_GRAPHICS_SCENE_CAMERA2D_H