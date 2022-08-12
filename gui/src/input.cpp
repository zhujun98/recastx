#include "input.hpp"
#include "input_handler.hpp"

#include <GLFW/glfw3.h>

namespace tomop::gui {

Input::Input(GLFWwindow* window) : window_(window) {
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
}

Input::~Input() = default;

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    for (auto& handler : instance(window).handlers_) {
        if (handler->handleMouseButton(button, action)) return;
    }
}

void Input::scrollCallback(GLFWwindow* window, double, double yoffset) {
    for (auto& handler : instance(window).handlers_) {
        if (handler->handleScroll(yoffset)) return;
    }
}

void Input::keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    for (auto& handler : instance(window).handlers_) {
        if (handler->handleKey(key, action, mods)) return;
    }
}

void Input::charCallback(GLFWwindow* window, unsigned int c) {
    for (auto& handler : instance(window).handlers_) {
        if (handler->handleChar(c)) return;
    }
}

void Input::tick(float /* time_elapsed */) {
    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfwGetCursorPos(window_, &mouse_x, &mouse_y);

    int w = 0;
    int h = 0;
    glfwGetWindowSize(window_, &w, &h);
    mouse_x = (2.0 * (mouse_x / w) - 1.0) * ((float)w / h);
    mouse_y = 2.0 * (mouse_y / h) - 1.0;

    for (auto& handler : instance(window_).handlers_) {
        if (handler->handleMouseMoved(mouse_x, mouse_y)) return;
    }
}

void Input::registerHandler(InputHandler& handler) {
    handlers_.insert(&handler);
}

} // tomop::gui
