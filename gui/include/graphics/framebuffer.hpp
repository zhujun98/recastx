/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_FRAMEBUFFER_H
#define GUI_FRAMEBUFFER_H

#include <iostream>

#include <GL/gl3w.h>

namespace recastx::gui {

class Framebuffer {

    GLuint fbo_;
    GLuint rbo_;
    GLuint texture_;

    int width_;
    int height_;

  public:

    Framebuffer(int width = 1, int height = 1);

    ~Framebuffer();

    void bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    }

    void unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint texture() const { return texture_; }

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
};

} // namespace recastx::gui

#endif // GUI_FRAMEBUFFER_H