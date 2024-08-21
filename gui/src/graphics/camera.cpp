/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
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

const glm::mat4& Camera::viewMatrix() {
    if (!view_) {
        view_ = glm::lookAt(pos_, target_, up_) * rotation_;
    }
    return view_.value();
}

glm::vec3 Camera::viewDir() {
    const glm::mat4& view = viewMatrix();
    return {-view[0][2], -view[1][2], -view[2][2]};
}

float Camera::distance() {
    return dist_;
}

glm::vec3 Camera::pos() {
    return target_ - viewDir() * dist_;
}

bool Camera::mouseDragEvent(const MouseDragEvent& ev) {
    if ((ev.alt_pressed && ev.button == MouseButton::LEFT) || ev.button == MouseButton::MIDDLE) {
        rotate(ev.delta.x * mouse_move_sensitivity_, up_);
        rotate(-ev.delta.y * mouse_move_sensitivity_, right_);
        return true;
    }
    return false;
}

bool Camera::mouseScrollEvent(const MouseScrollEvent& ev) {
    float delta = static_cast<float>(ev.delta) * mouse_scroll_sensitivity_;

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

bool Camera::keyEvent(const KeyEvent& ev) {
    switch (ev.key) {
        case KeyName::A:
            rotate(-key_sensitivity_, up_);
            return true;
        case KeyName::D:
            rotate(key_sensitivity_, up_);
            return true;
        case KeyName::W:
            rotate(-key_sensitivity_, right_);
            return true;
        case KeyName::S:
            rotate(key_sensitivity_, right_);
            return true;
        case KeyName::SPACE:
            setPerspectiveView();
            return true;
        default:
            return false;
    }
}

void Camera::setFrontView() {
    view_.reset();

    rotation_ = glm::mat4(1.0f);
    dist_ = 2.5f;
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
