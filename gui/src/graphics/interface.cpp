#include <fstream>
#include <iostream>
#include <string>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "graphics/interface.hpp"

namespace tomcat::gui {

Interface::Interface(GLFWwindow* glfw_window, MainWindow& main_window) : window_(main_window) {
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(glfw_window, false);
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = false;
    io.IniFilename = ""; // prevent ini file
}

Interface::~Interface() { ImGui_ImplGlfw_Shutdown(); }

void Interface::render(glm::mat4) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    window_.describe();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Interface::handleMouseButton(int button, int action) {
    ImGui_ImplGlfw_MouseButtonCallback(nullptr, button, action, 0);
    auto io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool Interface::handleScroll(double offset) {
    ImGui_ImplGlfw_ScrollCallback(nullptr, 0.0, offset);
    auto io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool Interface::handleKey(int key, bool down, int /* mods */) {
    ImGui_ImplGlfw_KeyCallback(
        nullptr, key, 0, down ? GLFW_PRESS : GLFW_RELEASE, 0);
    auto io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

bool Interface::handleChar(unsigned int c) {
    ImGui_ImplGlfw_CharCallback(nullptr, c);
    auto io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

bool Interface::handleMouseMoved(double /* x */, double /* y */) {
    auto io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

} // tomcat::gui
