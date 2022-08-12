#ifndef GUI_GRAPHICS_RENDER_TARGET_H
#define GUI_GRAPHICS_RENDER_TARGET_H

#include <glm/glm.hpp>


namespace tomop::gui {

class RenderTarget {
  public:
    RenderTarget();
    virtual ~RenderTarget();

    virtual void render(glm::mat4 window_matrix) = 0;
    [[nodiscard]] virtual int zPriority() const { return 0; }
};

} // tomop::gui

#endif // GUI_GRAPHICS_RENDER_TARGET_H