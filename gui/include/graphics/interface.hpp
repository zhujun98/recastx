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

    bool handle_mouse_button(int button, bool down) override;
    bool handle_scroll(double offset) override;
    bool handle_key(int key, bool down, int mods) override;
    bool handle_char(unsigned int c) override;
    bool handle_mouse_moved(float x, float y) override;

    int priority() const override { return 1; }

  private:
    SceneList& scenes_;
    std::vector<Window*> windows_;
};

} // namespace gui
