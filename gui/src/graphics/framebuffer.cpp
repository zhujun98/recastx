/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/framebuffer.hpp"

namespace recastx::gui {

Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height) {
    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &texture_);
    glGenRenderbuffers(1, &rbo_);

    rescale_required_ = true;
}

Framebuffer::~Framebuffer() {
    glDeleteRenderbuffers(1, &rbo_);
    glDeleteFramebuffers(1, &fbo_);
}

void Framebuffer::prepareForRescale(int width, int height) {
    width_ = width;
    height_ = height;
    rescale_required_ = true;
}

void Framebuffer::rescale() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    rescale_required_ = false;
}

void Framebuffer::bind() {
    if (rescale_required_) rescale();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace recastx::gui
