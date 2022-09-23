#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "graphics/nodes/camera.hpp"

namespace tomcat::gui {

Camera::Camera() : center_( {0.f, 0.f, 0.f}) {
    setPerspectiveView();
}
Camera::~Camera() = default;

const glm::mat4& Camera::matrix() {
    if (view_) { return view_.value(); }

    view_ = glm::lookAt(pos_, center_, up_) * rotation_;
    return view_.value();
}

bool Camera::handleMouseButton(int /* button */, int action) {
    dragging_ = action == GLFW_PRESS;
    return true;
}

bool Camera::handleScroll(float offset) {
    // TODO: set min/max limits
    pos_ -= offset * mouse_scroll_sensitivity_ * glm::normalize(pos_);
    view_.reset();
    return true;
}

bool Camera::handleMouseMoved(float x, float y) {
    if (prev_y_ < -1.0f) {
        prev_x_ = x;
        prev_y_ = y;
    }

    float x_offset = x - prev_x_;
    float y_offset = prev_y_ - y;
    prev_x_ = x;
    prev_y_ = y;

    if (dragging_) {
        if (!fixed_) {
            adjustPitch(y_offset * mouse_move_sensitivity_);
            adjustYaw(x_offset * mouse_move_sensitivity_);
            view_.reset();
        }
        return true;
    }

    return false;
}

bool Camera::handleKey(int key, int action, int /* mods */) {
    // TODO: Implement press and hold
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_A:
                adjustYaw(-key_sensitivity_);
                view_.reset();
                return true;
            case GLFW_KEY_D:
                adjustYaw(key_sensitivity_);
                view_.reset();
                return true;
            case GLFW_KEY_W:
                adjustPitch(key_sensitivity_);
                view_.reset();
                return true;
            case GLFW_KEY_S:
                adjustPitch(-key_sensitivity_);
                view_.reset();
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
        pos_.z -= 5.0;
        up_ = glm::vec3(0.0f, 1.0f, 0.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
        view_.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Y-Z")) {
        rotation_ = glm::mat4(1.0f);
        pos_ = center_;
        pos_.x -= 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(0.0f, 1.0f, 0.0f);
        view_.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("X-Z")) {
        rotation_ = glm::mat4(1.0f);
        pos_ = center_;
        pos_.y += 5.0;
        up_ = glm::vec3(0.0f, 0.0f, 1.0f);
        right_ = glm::vec3(1.0f, 0.0f, 0.0f);
        view_.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Perspective")) {
        setPerspectiveView();
    }
}

void Camera::setPerspectiveView() {
    rotation_ = glm::mat4(1.0f);
    pos_ = center_;
    pos_.z -= 5.0;
    pos_.y += 2.5;
    pos_.x -= 2.5;
    up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    right_ = glm::vec3(1.0f, 0.0f, 0.0f);
    view_.reset();
}

void Camera::adjustPitch(float offset) {
    rotation_ = glm::rotate(offset, right_) * rotation_;
}

void Camera::adjustYaw(float offset) {
    rotation_ = glm::rotate(offset, up_) * rotation_;
}

} // tomcat::gui
