#pragma once

#include <glm/glm.hpp>


namespace gui {

class RenderTarget {
  public:
    virtual ~RenderTarget() = 0;
    virtual void render(glm::mat4 window_matrix) = 0;
    [[nodiscard]] virtual int zPriority() const { return 0; }
};

} // namespace gui
