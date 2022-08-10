#include <fstream>
#include <iostream>
#include <string>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "graphics/interface.hpp"

namespace gui {

Interface::Interface(GLFWwindow* window, SceneList& scenes) : scenes_(scenes) {
    // Setup ImGui binding
    ImGui::CreateContext();

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see
    // extra_fonts/README.txt for more details)
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init();

    std::string other_font = "../data/iosevka-medium.ttf";
    std::ifstream infile(other_font);
    if (infile.good()) {
        io.Fonts->AddFontFromFileTTF(other_font.c_str(), 20.0f);
    } else {
        std::cout << "Can not find Iosevka font, resorting back to default\n";
        io.Fonts->AddFontDefault();
    }
    io.MouseDrawCursor = false;

    ImGui::StyleColorsDark();

    // prevent ini file
    io.IniFilename = "";
}

Interface::~Interface() { ImGui_ImplGlfw_Shutdown(); }

void Interface::render(glm::mat4) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    scenes_.describe();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Interface::handleMouseButton(int button, bool down) {
    ImGui_ImplGlfw_MouseButtonCallback(
        nullptr, button, down ? GLFW_PRESS : GLFW_RELEASE, 0);
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

} // namespace gui
