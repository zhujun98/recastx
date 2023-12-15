/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <implot.h>

#include <spdlog/spdlog.h>

#include "application.hpp"
#include "rpc_client.hpp"
#include "graphics/scene3d.hpp"
#include "graphics/style.hpp"

namespace recastx::gui {

namespace detail {

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

} // detail

Application::Application() {
    glfwSetErrorCallback(detail::glfwErrorCallback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // Do we need it?
    glfwWindowHint(GLFW_SAMPLES, 4);

    glfw_window_ = glfwCreateWindow(width_, height_, title_.c_str(), NULL, NULL);
    glfwMakeContextCurrent(glfw_window_);
    glfwSetWindowSizeLimits(glfw_window_, min_width_, min_height_, GLFW_DONT_CARE, GLFW_DONT_CARE);

    if (gl3wInit()) {
        throw std::runtime_error("Failed to initialize OpenGL!");
    }

    spdlog::info("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    initImgui();
    registerCallbacks();
}

Application::~Application() {
    shutdownImgui();

    glfwDestroyWindow(glfw_window_);
    glfwTerminate();
}

Application& Application::instance() {
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Application>(new Application);
    }
    return *instance_;
}

void Application::makeScene(){
    scene_ = std::make_unique<Scene3d>(rpc_client_.get());

    glfwGetWindowSize(glfw_window_, &width_, &height_);
    scene_->onWindowSizeChanged(width_, height_);

    int display_w, display_h;
    glfwGetFramebufferSize(glfw_window_, &display_w, &display_h);
    scene_->onFramebufferSizeChanged(display_w, display_h);
}

void Application::spin(const std::string& endpoint) {

    rpc_client_ = std::make_unique<RpcClient>(endpoint);

    makeScene();

    while (!glfwWindowShouldClose(glfw_window_)) {
        glfwPollEvents();
        scene_->consumeData();
        render();
    }
}

void Application::initImgui() {
    ImGui::CreateContext();
    ImPlot::CreateContext();

    Style::init();

    // TODO: why false?
    ImGui_ImplGlfw_InitForOpenGL(glfw_window_, false);
    ImGui_ImplOpenGL3_Init();

    // Do we need them?
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = false;
    io.IniFilename = ""; // prevent ini file
}

void Application::shutdownImgui() {
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::registerCallbacks() {
    glfwSetWindowSizeCallback(glfw_window_, [](GLFWwindow* /*window*/, int width, int height) {
        instance().scene_->onWindowSizeChanged(width, height);
    });

    glfwSetFramebufferSizeCallback(glfw_window_, [](GLFWwindow* /*window*/, int width, int height) {
        instance().scene_->onFramebufferSizeChanged(width, height);
    });

    glfwSetMouseButtonCallback(glfw_window_, mouseButtonCallback);
    glfwSetCursorPosCallback(glfw_window_, cursorPosCallback);
    glfwSetScrollCallback(glfw_window_, scrollCallback);
    glfwSetKeyCallback(glfw_window_, keyCallback);
    glfwSetCharCallback(glfw_window_, charCallback);
}

void Application::mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int) {
    ImGui_ImplGlfw_MouseButtonCallback(nullptr, button, action, 0);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        instance().scene_->handleMouseButton(button, action);
    }
}

void Application::scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(nullptr, 0.0, yoffset);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        instance().scene_->handleScroll(static_cast<float>(yoffset));
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ImGui_ImplGlfw_CursorPosCallback(nullptr, xpos, ypos);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        auto [x, y] = Application::normalizeCursorPos(window, xpos, ypos);
        instance().scene_->handleMouseMoved(x, y);
    }
}

void Application::keyCallback(GLFWwindow* /*window*/, int key, int, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(nullptr, key, 0, action, 0);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        instance().scene_->handleKey(key, action, mods);
    }
}

void Application::charCallback(GLFWwindow* /*window*/, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(nullptr, c);
    auto io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        instance().scene_->handleChar(c);
    }
}

void Application::render() {
    glClearColor(bg_color_.x, bg_color_.y, bg_color_.z, bg_color_.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    scene_->render();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(glfw_window_);
}

std::array<float, 2> Application::normalizeCursorPos(GLFWwindow* window, double xpos, double ypos) {
    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    xpos = (2.0 * (xpos / w) - 1.0) * w / h;
    ypos = 2.0 * (ypos / h) - 1.0;
    return {static_cast<float>(xpos), static_cast<float>(ypos)};
}

} // namespace recastx::gui
