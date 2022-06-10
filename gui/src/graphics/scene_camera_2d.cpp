#include "graphics/scene_camera_2d.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

namespace gui {

SceneCamera2d::SceneCamera2d() = default;

glm::mat4 SceneCamera2d::matrix() {
    glm::mat4 camera_matrix;

    camera_matrix = glm::scale(glm::vec3(glm::vec2(scale_), 1.0f)) * camera_matrix;

    camera_matrix = glm::rotate(angle_, glm::vec3(0.0f, 0.0f, 1.0f)) * camera_matrix;

    camera_matrix = glm::translate(position_) * camera_matrix;

    return camera_matrix;
}

bool SceneCamera2d::handleMouseButton(int /* button */, bool down) {
    dragging_ = down;
    return true;
}

bool SceneCamera2d::handleScroll(double offset) {
    scale_ += offset / 20.0;
    return true;
}

bool SceneCamera2d::handleKey(int key, bool down, int /* mods */) {
    float offset = 0.05f;
    if (down) {
        switch (key) {
        case GLFW_KEY_H:
            position_.x += offset;
            return true;
        case GLFW_KEY_L:
            position_.x -= offset;
            return true;
        case GLFW_KEY_K:
            position_.y -= offset;
            return true;
        case GLFW_KEY_J:
            position_.y += offset;
            return true;
        case GLFW_KEY_EQUAL:
            scale_ *= 1.1f;
            return true;
        case GLFW_KEY_MINUS:
            scale_ /= 1.1f;
            return true;
        default:
            break;
        }
    }
    return false;
}

bool SceneCamera2d::handleMouseMoved(double x, double y) {
    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    // TODO: fix for screen ratio ratio
    if (dragging_) {
        position_.x += x - prev_x_;
        position_.y -= y - prev_y_;

        prev_x_ = x;
        prev_y_ = y;

        return true;
    }

    prev_x_ = x;
    prev_y_ = y;

    return false;
}

} // namespace gui
