#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "graphics/nodes/scene_camera3d.hpp"

namespace tomcat::gui {

// class Rotator

Rotator::Rotator(SceneCamera3d& camera, float x, float y, bool instant)
    : Rotator(camera) {
    x_ = x;
    y_ = y;
    cx_ = x;
    cy_ = y;
    instant_ = instant;
}

void Rotator::onDrag(glm::vec2 cur, glm::vec2 delta) {
    if (instant_) {
      camera_.rotate(3.0f * delta.x, -3.0f * delta.y);
    } else {
      cx_ = cur.x;
      cy_ = cur.y;
    }
}

void Rotator::tick(double time_elapsed) {
    if (instant_) return;

    camera_.rotate(10.0f * time_elapsed * (cx_ - x_), -10.0f * time_elapsed * (cy_ - y_));
}

drag_machine_kind Rotator::kind() {
    return drag_machine_kind::rotator;
}

// class SceneCamera3d

SceneCamera3d::SceneCamera3d() {
    setPerspectiveView();
};
SceneCamera3d::~SceneCamera3d() = default;

void SceneCamera3d::lookAt(glm::vec3 center) { center_ = std::move(center); }

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
        drag_machine_->onDrag({x, y}, delta);
        return true;
    }

    return false;
}

bool SceneCamera3d::handleKey(int key, int action, int /* mods */) {
    if (interaction_disabled_) return false;

    float offset = 0.05f;
    if (action == GLFW_PRESS) {
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
                setPerspectiveView();
                return true;
            default:
                break;
        }
    }
    return false;
}

void SceneCamera3d::renderIm() {
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
        setPerspectiveView();
    }
}

void SceneCamera3d::tick(double time_elapsed) {
    if (dragging_) drag_machine_->tick(time_elapsed);
}

void SceneCamera3d::setPerspectiveView() {
    rotation_ = glm::mat4(1.0f);
    position_ = center_;
    position_.z += 5.0;
    position_.y += 2.5;
    position_.x += 2.5;
    up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    right_ = glm::vec3(1.0f, 0.0f, 0.0f);
}

} // tomcat::gui
