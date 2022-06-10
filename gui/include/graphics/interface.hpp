#pragma once

#include <vector>

#include "graphics/render_target.hpp"
#include "input_handler.hpp"
#include "scene.hpp"


struct GLFWwindow;

namespace gui {

class Window;

class Interface : public RenderTarget, public InputHandler {
  public:
    Interface(GLFWwindow* window, SceneList& scenes);
    ~Interface() override;

    void render(glm::mat4) override;

    [[nodiscard]] int zPriority() const override { return 10; }

    bool handleMouseButton(int button, bool down) override;
    bool handleScroll(double offset) override;
    bool handleKey(int key, bool down, int mods) override;
    bool handleChar(unsigned int c) override;
    bool handleMouseMoved(double x, double y) override;

    int priority() const override { return 1; }

  private:
    SceneList& scenes_;
    std::vector<Window*> windows_;
};

} // namespace gui
