/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

#include "graphics/style.hpp"

struct GLFWwindow;

namespace recastx::gui {

class Scene;

class Application {

    static constexpr int min_width_ = 800;
    static constexpr int min_height_ = 600;

    int width_ = 1440;
    int height_ = 1080;
    const std::string title_ = "RECASTX - REConstruction of Arbitrary Slabs in Tomography X";

    ImVec4 bg_color_ { Style::BG_COLOR };

    GLFWwindow* glfw_window_ = nullptr;

    inline static std::unique_ptr<Application> instance_;

    Scene* scene_;

    Application();

    void initImgui();

    void shutdownImgui();

    void registerCallbacks();

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int);

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset);

    static void keyCallback(GLFWwindow* window, int key, int, int action, int mods);

    static void charCallback(GLFWwindow* window, unsigned int c);

    void render();

    static std::array<float, 2> normalizeCursorPos(GLFWwindow* window, double xpos, double ypos);

public:

    ~Application();

    static Application& instance();

    void setScene(Scene* scene);

    void spin();
};

}  // namespace recastx::gui

#endif // GUI_APPLICATION_H