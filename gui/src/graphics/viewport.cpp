#include <GL/gl3w.h>
#include <glm/gtx/transform.hpp>

#include "graphics/viewport.hpp"

namespace tomcat::gui {

Viewport::Viewport() = default;

Viewport::~Viewport() = default;

void Viewport::update(int x, int y, int w, int h) {
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;

    projection_ = glm::perspective(glm::radians(45.f), (float)w / (float)h, 0.1f, 50.0f);
}

void Viewport::use() const {
    glViewport(x_, y_, w_, h_);
}

} // namespace tomcat::gui
