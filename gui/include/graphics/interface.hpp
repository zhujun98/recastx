#pragma once

#include <vector>

#include "graphics/render_target.hpp"
#include "input_handler.hpp"
#include "window.hpp"


struct GLFWwindow;

namespace tomcat::gui {

class Interface : public RenderTarget, public InputHandler {

    MainWindow& window_;

  public:

    Interface(GLFWwindow* glfw_window, MainWindow& main_window);
    ~Interface() override;

    void render(glm::mat4) override;

    [[nodiscard]] int zPriority() const override { return 10; }

    bool handleMouseButton(int button, int action) override;
    bool handleScroll(double offset) override;
    bool handleKey(int key, bool down, int mods) override;
    bool handleChar(unsigned int c) override;
    bool handleMouseMoved(double x, double y) override;

    [[nodiscard]] int priority() const override { return 1; }
};

} // tomcat::gui
