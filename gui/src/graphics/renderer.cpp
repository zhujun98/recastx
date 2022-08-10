#include <algorithm>
#include <stdio.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

#include "graphics/renderer.hpp"

namespace gui {

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description);
}

Renderer::Renderer() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        throw;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(1920, 1080, "TOMCAT 3D Live Reconstruction", NULL, NULL);
    glfwMakeContextCurrent(window_);

    gl3wInit();
    glEnable(GL_MULTISAMPLE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    previous_time_ = glfwGetTime();
}

Renderer::~Renderer() { glfwTerminate(); }

void Renderer::spin() {
    ImVec4 bg_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        auto current_time = glfwGetTime();
        float time_elapsed = current_time - previous_time_;
        previous_time_ = current_time;

        // FIXME: this does not look like a proper event loop
        const auto time_step = 0.0166666666;
        for (auto ticker : tickers_) {
            while (time_elapsed > time_step) {
                ticker->tick(time_step);
                time_elapsed -= time_step;
            }
            ticker->tick(time_elapsed);
        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(bg_color.x, bg_color.y, bg_color.z, bg_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float ratio = (float)display_h / (float)display_w;

        auto window_matrix = glm::scale(glm::vec3(ratio, 1.0, 1.0));

        for (auto target : targets_) target->render(window_matrix);

        glfwSwapBuffers(window_);
    }
}

void Renderer::register_target(RenderTarget& target) { targets_.insert(&target); }

void Renderer::register_ticker(Ticker& ticker) { tickers_.push_back(&ticker); }

} // namespace gui
