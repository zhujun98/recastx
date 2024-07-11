/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "graphics/camera.hpp"

namespace recastx::gui {

Camera::Camera()
        : target_({0.f, 0.f, 0.f}),
          front_({-1.f, 0.f, 0.f}),
          up_({0.0f, 0.0f, 1.0f}),
          right_(glm::normalize(glm::cross(front_, up_))) {
    setPerspectiveView();
}

Camera::~Camera() = default;

const glm::mat4& Camera::matrix() {
    if (!view_) {
        view_ = glm::lookAt(pos_, target_, up_) * rotation_;
    }
    return view_.value();
}

glm::vec3 Camera::viewDir() {
    const glm::mat4& view = matrix();
    return {-view[0][2], -view[1][2], -view[2][2]};
}

float Camera::distance() {
    return dist_;
}

glm::vec3 Camera::pos() {
    return target_ - viewDir() * dist_;
}

bool Camera::handleMouseButton(int button, int action) {
    dragging_ = action == GLFW_PRESS && (
            (alt_pressed_ && button == GLFW_MOUSE_BUTTON_LEFT)
            || button == GLFW_MOUSE_BUTTON_MIDDLE
    );
    return true;
}

bool Camera::handleScroll(float offset) {
    float delta = offset * mouse_scroll_sensitivity_;

    if (dist_ + delta > K_MAX_DIST_) {
        dist_ = K_MAX_DIST_;
    } else if (dist_ + delta < K_MIN_DIST_) {
        dist_ = K_MIN_DIST_;
    } else {
        dist_ += delta;
    }

    pos_ = target_ - dist_ * front_;
    view_.reset();
    return true;
}

bool Camera::handleMouseMoved(float x, float y) {
    if (!initialized_) {
        prev_x_ = x;
        prev_y_ = y;
        initialized_ = true;
    }

    float x_offset = x - prev_x_;
    float y_offset = prev_y_ - y;
    prev_x_ = x;
    prev_y_ = y;

    if (dragging_) {
        rotate(x_offset * mouse_move_sensitivity_, up_);
        rotate(-y_offset * mouse_move_sensitivity_, right_);
        return true;
    }

    return false;
}

bool Camera::handleKey(int key, int action, int /* mods */) {
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_LEFT_ALT:
                alt_pressed_ = true;
                return true;
            case GLFW_KEY_A:
                rotate(-key_sensitivity_, up_);
                return true;
            case GLFW_KEY_D:
                rotate(key_sensitivity_, up_);
                return true;
            case GLFW_KEY_W:
                rotate(-key_sensitivity_, right_);
                return true;
            case GLFW_KEY_S:
                rotate(key_sensitivity_, right_);
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
    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT_ALT:
                alt_pressed_ = false;
                return true;
            default:
                break;
        }
    }
    return false;
}

void Camera::setFrontView() {
    view_.reset();

    rotation_ = glm::mat4(1.0f);
    dist_ = 4.5f;
    pos_ = glm::vec3{dist_, 0.f, 0.f};
}

void Camera::setTopView() {
    setFrontView();

    rotate(glm::radians(90.f), right_);
}

void Camera::setSideView() {
    setFrontView();

    rotate(glm::radians(90.f), -up_);
}

void Camera::setPerspectiveView() {
    setFrontView();

    rotate(glm::radians(30.f), -up_);
    rotate(glm::radians(30.f), right_);
}

void Camera::rotate(float angle, const glm::vec3& axis) {
    rotation_ = glm::rotate(angle, axis) * rotation_;
    view_.reset();
}

} // namespace recastx::gui
