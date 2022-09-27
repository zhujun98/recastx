#include <GL/gl3w.h>
#include <glm/gtx/transform.hpp>

#include "graphics/viewport.hpp"

namespace tomcat::gui {

Viewport::Viewport(bool perspective, float near, float far)
    : perspective_(perspective), near_(near), far_(far) {};

Viewport::~Viewport() = default;

void Viewport::update(int x, int y, int w, int h) {
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;

    projection_.reset();
}

void Viewport::use() const {
    glViewport(x_, y_, w_, h_);
}

[[nodiscard]] const glm::mat4& Viewport::projection() {
    if (!projection_) {
        float asp = (float)w_ / (float)h_;
        if (perspective_) {
            projection_ = glm::perspective(fov_, asp, near_, far_);
        } else {
            projection_ = glm::ortho(-asp, asp, -1.f, 1.f, -far_, far_);
        }
    }
    return projection_.value();
}


} // namespace tomcat::gui
