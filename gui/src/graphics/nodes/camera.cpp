#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "graphics/nodes/camera.hpp"

namespace tomcat::gui {

Camera::Camera() {
    setPerspectiveView();
}
Camera::~Camera() = default;

void Camera::rotate(float phi, float psi) {
    auto rotate_up = glm::rotate(phi, up_);
    auto rotate_right = glm::rotate(psi, right_);
    rotation_ = rotate_up * rotate_right * rotation_;
}

glm::mat4 Camera::matrix() {
    glm::mat4 camera_matrix = glm::lookAt(pos_, center_, up_);

    camera_matrix *= rotation_;

    return camera_matrix;
}

bool Camera::handleMouseButton(int /* button */, int action) {
    dragging_ = action == GLFW_PRESS;
    return true;
}

bool Camera::handleScroll(double offset) {
    pos_ *= (1.0 - offset / 20.0);
    return true;
}

bool Camera::handleMouseMoved(double x, double y) {
    // update slices that is being hovered over
    y = -y;

    if (prev_y_ < -1.0) {
        prev_x_ = x;
        prev_y_ = y;
    }

    float x_offset = x - prev_x_;
    float y_offset = prev_y_ - y;
    prev_x_ = x;
    prev_y_ = y;

    // TODO: fix for screen ratio
    if (dragging_) {
        if (!fixed_) {
            rotate(mouse_sensitivity_ * x_offset, mouse_sensitivity_ * y_offset);
        }
        return true;
    }

    return false;
}

bool Camera::handleKey(int key, int action, int /* mods */) {
    float offset = 0.05f;
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_H:
                pos_.x -= offset;
                return true;
            case GLFW_KEY_L:
                pos_.x += offset;
                return true;
            case GLFW_KEY_K:
                pos_.y += offset;
                return true;
            case GLFW_KEY_J:
                pos_.y -= offset;
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

void Camera::renderIm() {
    ImGui::Checkbox("Fix camera", &fixed_);

    if (ImGui::Button("X-Y")) {
        rotation_ = glm::mat4(1.0f);
        pos_ = center_;
        pos_.z += 5.0;
        up_ = glm::vec3(0.0f, 1.0f, 0.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        rotation_ = glm::mat4(1.0f);
        pos_ = center_;
        pos_.x += 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        rotation_ = glm::mat4(1.0f);
        pos_ = center_;
        pos_.y -= 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective")) {
        setPerspectiveView();
    }
}

void Camera::setPerspectiveView() {
    rotation_ = glm::mat4(1.0f);
    pos_ = center_;
    pos_.z += 5.0;
    pos_.y += 2.5;
    pos_.x += 2.5;
    up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    right_ = glm::vec3(1.0f, 0.0f, 0.0f);
}

} // tomcat::gui
