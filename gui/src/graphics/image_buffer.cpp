/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/image_buffer.hpp"
#include "graphics/aesthetics.hpp"

#include "graphics/shader_program.hpp"

namespace recastx::gui {

ImageBuffer::ImageBuffer() {
    static const GLfloat square[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &texture_);
    glGenRenderbuffers(1, &rbo_);

    init();

    auto vert =
#include "shaders/image_buffer.vert"
    ;
    auto frag =
#include "shaders/image_buffer.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

ImageBuffer::~ImageBuffer() {
    glDeleteBuffers(1, &rbo_);
    glDeleteBuffers(1, &fbo_);

    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
}

void ImageBuffer::render(int width, int height, float min_v, float max_v) {
    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("imageTexture", 1);
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    auto [w, h] = computeImageSize(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    clearImp();

    glViewport((width_ - w) / 2, (height_ - h) / 2, w, h);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shader_->unuse();
}

void ImageBuffer::clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    clearImp();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool ImageBuffer::resize(int width, int height) {
    if (width_ != width || height_ != height) {
        width_ = width;
        height_ = height;
        init();
        return true;
    }
    return false;
}

void ImageBuffer::clearImp() const {
    glViewport(0, 0, width_, height_);
    glClearColor(bg_.x, bg_.y, bg_.z, bg_.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ImageBuffer::init() const {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Framebuffer is not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::array<int, 2> ImageBuffer::computeImageSize(int width, int height) const {
    if (keep_aspect_ratio_) {
        double ratio = static_cast<double>(height) / width;
        if (width_ * ratio > height_) {
            return {static_cast<int>(height_ / ratio), height_};
        }
        return {width_, static_cast<int>(width_ * ratio)};
    }
    return {width_, height_};
}

} // namespace recastx::gui
