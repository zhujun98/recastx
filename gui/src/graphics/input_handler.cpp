/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/input_handler.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

InputHandler::InputHandler() = default;

InputHandler::~InputHandler() = default;

void InputHandler::onWindowSizeChanged(int width, int height) {
    width_ = width;
    height_ = height;
}

void InputHandler::handle(const std::vector<std::unique_ptr<Scene>> &scenes) {
    while (!events_.empty()) {
        for (auto &scene: scenes) {
            if (scene->consumeEvent(events_.front())) break;
        }
        events_.pop();
    }
}

void InputHandler::registerMouseButton(int button, int action) {
    if (action == GLFW_RELEASE) {
        if (dragging_) {
            MouseDragEvent ev;
            ev.pos = {prev_x_, prev_y_};
            ev.delta = {0, 0};
            ev.button = prev_pressed_;
            ev.alt_pressed = alt_pressed_;
            ev.exiting = true;
            events_.emplace(ev);

            dragging_ = false;
        } else {
            MouseClickEvent ev;
            ev.pos = { prev_x_, prev_y_ };
            ev.button = prev_pressed_;
            events_.emplace(ev);
        }
        prev_pressed_ = MouseButton::NONE;

    } else if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) prev_pressed_ = MouseButton::LEFT;
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) prev_pressed_ = MouseButton::MIDDLE;
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) prev_pressed_ = MouseButton::RIGHT;
    }
}

void InputHandler::registerMouseScroll(double yoffset) {
    MouseScrollEvent ev;
    ev.pos = {prev_x_, prev_y_};
    ev.delta = yoffset;

    events_.emplace(std::move(ev));
}

void InputHandler::registerMouseMove(double xpos, double ypos) {
    double x = 2. * xpos / width_ - 1.;
    double y = 1. - 2. * ypos / height_;

    if (!initialized_) {
        prev_x_ = x;
        prev_y_ = y;
        dx_ = 0.f;
        dy_ = 0.f;
        initialized_ = true;
    }

    dx_ = x - prev_x_;
    dy_ = y - prev_y_;
    prev_x_ = x;
    prev_y_ = y;

    if (prev_pressed_ != MouseButton::NONE) {
        MouseDragEvent ev;
        ev.pos = {prev_x_, prev_y_};
        ev.delta = {dx_, dy_};
        ev.button = prev_pressed_;
        ev.alt_pressed = alt_pressed_;

        if (!dragging_) {
            ev.entering = true;
            dragging_ = true;
        }

        events_.emplace(ev);
    } else {
        MouseHoverEvent ev;
        ev.pos = {prev_x_, prev_y_};

        events_.emplace(std::move(ev));
    }
}

void InputHandler::registerKey(int key, int action, int /*mods*/) {
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        KeyEvent ev;
        if (action == GLFW_REPEAT) ev.repeat = true;

        switch (key) {
            case GLFW_KEY_LEFT_ALT:
                alt_pressed_ = true;
                break;
            case GLFW_KEY_A:
                ev.key = KeyName::A;
                break;
            case GLFW_KEY_D:
                ev.key = KeyName::D;
                break;
            case GLFW_KEY_W:
                ev.key = KeyName::W;
                break;
            case GLFW_KEY_S:
                ev.key = KeyName::S;
                break;
            case GLFW_KEY_SPACE:
                ev.key = KeyName::SPACE;
                break;
            default:
                return;
        }

        events_.emplace(std::move(ev));

    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_LEFT_ALT:
                alt_pressed_ = false;
                break;
            default:
                return;
        }
    }
}

} // recastx::gui