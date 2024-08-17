/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_INPUT_HANDLER_H
#define GUI_INPUT_HANDLER_H

#include <memory>
#include <queue>
#include <tuple>
#include <unordered_map>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "viewport.hpp"
#include "interactable.hpp"
#include "event.hpp"

namespace recastx::gui {

class Scene;

class InputHandler {

    int width_{0};
    int height_{0};

    bool initialized_ = false;
    double prev_x_ = 0.f;
    double prev_y_ = 0.f;
    double dx_ = 0.f;
    double dy_ = 0.f;

    MouseButton prev_pressed_ = MouseButton::NONE;
    bool dragging_ { false };
    bool alt_pressed_ { false };
    std::queue<InputEvent> events_;

  public:

    InputHandler();

    ~InputHandler();

    void onWindowSizeChanged(int width, int height);

    void handle(const std::vector<std::unique_ptr<Scene>> &scenes);

    void registerMouseButton(int button, int action);

    void registerMouseScroll(double yoffset);

    void registerMouseMove(double xpos, double ypos);

    void registerKey(int key, int action, int /*mods*/);

};

} // recastx::gui

#endif //GUI_INPUT_HANDLER_H
