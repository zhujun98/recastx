/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_EVENT_H
#define GUI_EVENT_H

#include <variant>

#include <glm/glm.hpp>

enum class MouseButton { NONE, LEFT, MIDDLE, RIGHT};
enum class KeyName { NONE, A, S, D, W, ALT, SPACE};

enum class EventType { DRAG, HOVER, SCROLL, KEY };

struct MouseDragEvent {
    glm::vec2 pos;
    glm::vec2 delta;
    MouseButton button { MouseButton::NONE };
    bool alt_pressed { false };
    bool entering { false };
    bool exiting { false };
};

struct MouseHoverEvent {
    glm::vec2 pos;
    bool entering { false };
    bool exiting { false };
};

struct MouseClickEvent {
    glm::vec2 pos;
    MouseButton button { MouseButton::NONE };
};

struct MouseScrollEvent {
    glm::vec2 pos;
    double delta { 0. };
};

struct KeyEvent {
    KeyName key;
    bool repeat { false };
};

using InputEvent = std::variant<MouseDragEvent, MouseHoverEvent, MouseClickEvent, MouseScrollEvent, KeyEvent>;

#endif //GUI_EVENT_H
