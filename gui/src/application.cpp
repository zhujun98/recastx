#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_glfw.h"
#include "implot.h"

#include "application.hpp"
#include "graphics/scenes/scene3d.hpp"

namespace tomcat::gui {

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

    if (gl3wInit()) {
        throw std::runtime_error("Failed to initialize OpenGL!");
    }
    // Do we need them?
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initImgui();
    registerCallbacks();

}

Application::~Application() {
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(glfw_window_);
    glfwTerminate();
}

Application& Application::instance() {
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Application>(new Application);
    }
    return *instance_;
}

void Application::setScene(Scene3d* scene) { scene_ = scene; }

void Application::start() {
    double prev_time = glfwGetTime();
    while (!glfwWindowShouldClose(glfw_window_)) {
        glfwPollEvents();

        double curr_time = glfwGetTime();
        double time_elapsed = curr_time - prev_time;
        prev_time = curr_time;

        // FIXME: this does not look like a proper event loop
        const auto time_step = 0.0166666666;
        while (time_elapsed > time_step) {
            scene_->tick(time_step);
            time_elapsed -= time_step;
        }
        scene_->tick(time_elapsed);

        render();
    }
}

void Application::initImgui() {
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui::StyleColorsDark();

    // TODO: why false?
    ImGui_ImplGlfw_InitForOpenGL(glfw_window_, false);
    ImGui_ImplOpenGL3_Init();

    // Do we need them?
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = false;
    io.IniFilename = ""; // prevent ini file
}

void Application::registerCallbacks() {
    glfwSetMouseButtonCallback(glfw_window_, mouseButtonCallback);
    glfwSetCursorPosCallback(glfw_window_, cursorPosCallback);
    glfwSetScrollCallback(glfw_window_, scrollCallback);
    glfwSetKeyCallback(glfw_window_, keyCallback);
    glfwSetCharCallback(glfw_window_, charCallback);
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    ImGui_ImplGlfw_MouseButtonCallback(nullptr, button, action, 0);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        instance().scene_->handleMouseButton(button, action);
    }
}

void Application::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(nullptr, 0.0, yoffset);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        instance().scene_->handleScroll(yoffset);
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

void Application::keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(nullptr, key, 0, action, 0);
    auto io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        instance().scene_->handleKey(key, action, mods);
    }
}

void Application::charCallback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(nullptr, c);
    auto io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        instance().scene_->handleChar(c);
    }
}

void Application::render() {
    int width, height;
    glfwGetFramebufferSize(glfw_window_, &width, &height);
    glViewport(0, 0, width, height);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    scene_->describe();

    ImGui::Render();

    glClearColor(bg_color_.x, bg_color_.y, bg_color_.z, bg_color_.w);
    // Why depth buffer bit?
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    scene_->render(glm::scale(glm::vec3((float)height / (float)width, 1.0, 1.0)));

    glfwSwapBuffers(glfw_window_);
}

std::array<double, 2> Application::normalizeCursorPos(GLFWwindow* window, double xpos, double ypos) {
    int w = 0;
    int h = 0;
    glfwGetWindowSize(window, &w, &h);

    xpos = (2.0 * (xpos / w) - 1.0) * (static_cast<double>(w) / h);
    ypos = 2.0 * (ypos / h) - 1.0;
    return {xpos, ypos};
}

} // namespace tomcat::gui
