#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

struct GLFWwindow;

namespace recastx::gui {

class Scene3d;

class Application {

    static constexpr int min_width_ = 640;
    static constexpr int min_height_ = 480;

    int width_ = 1440;
    int height_ = 1080;
    const std::string title_ = "RECASTX - Quasi-3D Real-time Tomographic Reconstruction";

    ImVec4 bg_color_ = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    GLFWwindow* glfw_window_ = nullptr;

    inline static std::unique_ptr<Application> instance_;

    Scene3d* scene_;

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

    void setScene(Scene3d* scene);

    void exec();
};

}  // namespace recastx::gui

#endif // GUI_APPLICATION_H