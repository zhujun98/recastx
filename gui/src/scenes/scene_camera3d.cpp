#include <imgui.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "scenes/scene_camera3d.hpp"

namespace gui {

// class Rotator

Rotator::Rotator(SceneCamera3d& camera, float x, float y, bool instant)
    : Rotator(camera) {
    x_ = x;
    y_ = y;
    cx_ = x;
    cy_ = y;
    instant_ = instant;
}

void Rotator::on_drag(glm::vec2 cur, glm::vec2 delta) {
    if (instant_) {
      camera_.rotate(3.0f * delta.x, -3.0f * delta.y);
    } else {
      cx_ = cur.x;
      cy_ = cur.y;
    }
}

void Rotator::tick(float time_elapsed) {
    if (instant_) return;

    camera_.rotate(10.0f * time_elapsed * (cx_ - x_), -10.0f * time_elapsed * (cy_ - y_));
}

drag_machine_kind Rotator::kind() {
    return drag_machine_kind::rotator;
}

// class SceneCamera3d

SceneCamera3d::SceneCamera3d() { reset_view(); }

void SceneCamera3d::reset_view() {
    // explicitly set to identity
    position_ = glm::vec3(0.0f, 0.0f, 5.0f);
    up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    rotation_ = glm::mat4(1.0f);

    SceneCamera3d::rotate(-0.25f * glm::pi<float>(), 0.0f);
}

void SceneCamera3d::set_look_at(glm::vec3 center) { center_ = center; }

void SceneCamera3d::set_position(glm::vec3 position) { position_ = position; }

void SceneCamera3d::set_right(glm::vec3 right) { right_ = right; }

void SceneCamera3d::set_up(glm::vec3 up) { up_ = up; }

void SceneCamera3d::rotate(float phi, float psi) {
    auto rotate_up = glm::rotate(phi, up_);
    auto rotate_right = glm::rotate(psi, right_);
    rotation_ = rotate_up * rotate_right * rotation_;
}

glm::mat4 SceneCamera3d::matrix() {
    glm::mat4 camera_matrix = glm::lookAt(position_, center_, up_);

    camera_matrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 50.0f) *
                    camera_matrix * rotation_;

    return camera_matrix;
}

bool SceneCamera3d::handleMouseButton(int /* button */, int action) {
    if (interaction_disabled_) return false;

    dragging_ = action == GLFW_PRESS;
    if (!dragging_) {
        drag_machine_ = nullptr;
    } else {
        switch_if_necessary(drag_machine_kind::rotator);
    }
    return true;
}

bool SceneCamera3d::handleScroll(double offset) {
    if (interaction_disabled_) {
        return false;
    }

    position_ *= (1.0 - offset / 20.0);
    return true;
}

bool SceneCamera3d::handleKey(int key, bool down, int /* mods */) {
    if (interaction_disabled_) return false;

    float offset = 0.05f;
    if (down) {
        switch (key) {
        case GLFW_KEY_H:
            position_.x -= offset;
            return true;
        case GLFW_KEY_L:
            position_.x += offset;
            return true;
        case GLFW_KEY_K:
            position_.y += offset;
            return true;
        case GLFW_KEY_J:
            position_.y -= offset;
            return true;
        case GLFW_KEY_EQUAL:
            scale_ *= 1.1f;
            return true;
        case GLFW_KEY_MINUS:
            scale_ /= 1.1f;
            return true;
        case GLFW_KEY_SPACE:
            reset_view();
            return true;
        default:
            break;
        }
    }
    return false;
}

void SceneCamera3d::switch_if_necessary(drag_machine_kind kind) {
    if (!drag_machine_ || drag_machine_->kind() != kind) {
        switch (kind) {
          case drag_machine_kind::rotator:
              drag_machine_ = std::make_unique<Rotator>(*this, prev_x_, prev_y_, instant_);
              break;
          default:
              break;
        }
    }
}

bool SceneCamera3d::handleMouseMoved(double x, double y) {
    if (interaction_disabled_) return false;

    // update slices that is being hovered over
    y = -y;

    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    glm::vec2 delta(x - prev_x_, y - prev_y_);
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio ratio
    if (dragging_) {
        drag_machine_->on_drag({x, y}, delta);
        return true;
    }

    return false;
}

void SceneCamera3d::describe() {
    SceneCamera::describe();
    ImGui::Checkbox("Instant Camera", &instant_);

    if (ImGui::Button("X-Y")) {
        rotation_ = glm::mat4(1.0f);
        position_ = center_;
        position_.z += 5.0;
        up_ = glm::vec3(0.0f, 1.0f, 0.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        rotation_ = glm::mat4(1.0f);
        position_ = center_;
        position_.x += 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        rotation_ = glm::mat4(1.0f);
        position_ = center_;
        position_.y -= 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective")) {
        rotation_ = glm::mat4(1.0f);
        position_ = center_;
        position_.z += 5.0;
        position_.y += 2.5;
        position_.x += 2.5;
        up_ = glm::vec3(0.0f, 1.0f, 0.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    }
}

void SceneCamera3d::tick(float time_elapsed) {
    if (dragging_) drag_machine_->tick(time_elapsed);
}

} // namespace gui
