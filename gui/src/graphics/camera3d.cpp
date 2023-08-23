/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <ostream>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "graphics/camera3d.hpp"

namespace recastx::gui {

Camera::Camera(const glm::vec3& target) : target_(target) {
    setPerspectiveView();
}

Camera::~Camera() = default;

const glm::mat4& Camera::matrix() {
    if (!view_) {
        view_ = glm::lookAt(pos0_, target_, up0_) * rotation_;
        assert(glm::normalize(target_ - pos0_) == front0_);
    }

    return view_.value();
}

float Camera::distance() {
    if (!distance_) distance_ = glm::length(pos0_ - target_);
    return distance_.value();
}

bool Camera::handleMouseButton(int /* button */, int action) {
    dragging_ = action == GLFW_PRESS;
    return true;
}

bool Camera::handleScroll(float offset) {
    pos0_ += offset * mouse_scroll_sensitivity_ * front0_;

    static const float K_MAX_DIST = 10.f;
    static const float K_MIN_DIST = 0.5f;
    pos0_[0] = std::min(K_MAX_DIST, std::max(K_MIN_DIST, pos0_[0]));

    view_.reset();
    distance_.reset();
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
        adjustYaw(x_offset * mouse_move_sensitivity_);
        adjustPitch(y_offset * mouse_move_sensitivity_);
        return true;
    }

    return false;
}

bool Camera::handleKey(int key, int action, int /* mods */) {
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_A:
                adjustYaw(-key_sensitivity_);
                return true;
            case GLFW_KEY_D:
                adjustYaw(key_sensitivity_);
                return true;
            case GLFW_KEY_W:
                adjustPitch(key_sensitivity_);
                return true;
            case GLFW_KEY_S:
                adjustPitch(-key_sensitivity_);
                return true;
            default:
                break;
        }

        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_SPACE) {
                setPerspectiveView();
                return true;
            }
        }
    }
    return false;
}

void Camera::setFrontView() {
    view_.reset();

    rotation_ = glm::mat4(1.0f);
    pos0_ = target_;
    pos0_.x += 5.0;
    up0_ = glm::vec3(0.0f, 0.0f, 1.0f);
    right0_ = glm::vec3(0.0f, 1.0f, 0.0f);
    front0_ = glm::cross(up0_, right0_);
}

void Camera::setTopView() {
    setFrontView();

    adjustYaw(glm::radians(90.f));
    adjustPitch(glm::radians(-90.f));

// Equivalent to:
//    view_.reset();
//
//    rotation_ = glm::mat4(1.0f);
//    pos0_ = target_;
//    pos0_.z += 5.0;
//    up0_ = glm::vec3(0.0f, 1.0f, 0.0f);
//    right0_ = glm::vec3(1.0f, 0.0f, 0.0f);
//    front0_ = glm::cross(up0_, right0_);
}

void Camera::setSideView() {
    setFrontView();

    adjustYaw(glm::radians(90.f));

    // Equivalent to:

//    view_.reset();
//
//    rotation_ = glm::mat4(1.0f);
//    pos0_ = target_;
//    pos0_.y -= 5.0;
//    up0_ = glm::vec3(0.0f, 0.0f, 1.0f);
//    right0_ = glm::vec3(1.0f, 0.0f, 0.0f);
//    front0_ = glm::cross(up0_, right0_);
}

void Camera::setPerspectiveView() {
    setFrontView();
    adjustYaw(glm::radians(-45.f));
    adjustPitch(glm::radians(-30.f));
}

void Camera::adjustPitch(float offset) {
    rotation_ = glm::rotate(-offset, right0_) * rotation_;
    view_.reset();
}

void Camera::adjustYaw(float offset) {
    rotation_ = glm::rotate(offset, up0_) * rotation_;
    view_.reset();
}

} // namespace recastx::gui
