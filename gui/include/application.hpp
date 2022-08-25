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

namespace tomcat::gui {

class Scene3d;
class Server;

class Application {

    inline static int width_ = 1440;
    inline static int height_ = 1080;
    inline static std::string title_ = "TOMCAT 3D Live Reconstruction";

    inline static auto bg_color_ = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    GLFWwindow* glfw_window_ = nullptr;

    inline static std::unique_ptr<Application> instance_;

    Scene3d* scene_;

    Application();

    void initImgui();

    void registerCallbacks();

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int);

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset);

    static void keyCallback(GLFWwindow* window, int key, int, int action, int mods);

    static void charCallback(GLFWwindow* window, unsigned int c);

    void render();

    static std::array<double, 2> normalizeCursorPos(GLFWwindow* window, double xpos, double ypos);

public:

    ~Application();

    static Application& instance();

    void setScene(Scene3d* scene);

    void setPublisher(Server* server);

    void start();
};

}  // namespace tomcat::gui

#endif // GUI_APPLICATION_H