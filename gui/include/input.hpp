#pragma once

#include <set>

#include "input_handler.hpp"
#include "ticker.hpp"


struct GLFWwindow;

namespace gui {

class Input : public Ticker {

    explicit Input(GLFWwindow* window);
    ~Input();

    struct InputCompare {
      bool operator()(const InputHandler* lhs, const InputHandler* rhs) const {
        return lhs->priority() < rhs->priority();
      }
    };

    std::set<InputHandler*, InputCompare> handlers_;

    GLFWwindow* window_;

  public:

    Input(Input const&) = delete;
    void operator=(Input const&) = delete;

    static Input& instance(GLFWwindow* window) {
        static Input instance(window);
        return instance;
    }

    void registerHandler(InputHandler& handler);

    static void mouseButtonCallback(GLFWwindow*, int button, int action, int);
    static void scrollCallback(GLFWwindow*, double, double yoffset);
    static void keyCallback(GLFWwindow*, int key, int, int action, int mods);
    static void charCallback(GLFWwindow*, unsigned int c);

    void tick(float time_elapsed) override;
};

} // namespace gui
