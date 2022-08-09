#pragma once

#include <string>
#include <glm/glm.hpp>

#include "input_handler.hpp"
#include "ticker.hpp"

namespace gui {

class ObjectComponent : public InputHandler, public Ticker {
  public:
    ObjectComponent() = default;
    virtual ~ObjectComponent() = default;

    virtual void draw(glm::mat4 /* world_to_screen */) {}
    virtual std::string identifier() const = 0;
    void tick(float /* time_elapsed */) override {}
    virtual void describe() {}

    // low priority gets drawn last
    [[nodiscard]] int priority() const override { return 1; }
};

} // namespace gui
