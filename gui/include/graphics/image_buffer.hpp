/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_IMAGEBUFFER_HPP
#define GUI_IMAGEBUFFER_HPP

#include <array>
#include <memory>
#include <vector>

#include <imgui.h>
#include "GL/gl3w.h"

#include "style.hpp"

namespace recastx::gui {

class ShaderProgram;

class ImageBuffer {

    GLuint vao_;
    GLuint vbo_;

    int width_ { 8 };
    int height_ { 8 };
    ImVec4 bg_ { Style::IMAGE_BUFFER_BG_COLOR };

    GLuint fbo_;
    GLuint texture_;
    GLuint rbo_;

    bool keep_aspect_ratio_ { true };

    std::unique_ptr<ShaderProgram> shader_;

    void clearImp() const;

    void init() const;

    [[nodiscard]] std::array<int, 2> computeImageSize(int width, int height) const;

  public:

    ImageBuffer();
    ~ImageBuffer();

    void render(int width, int height, float min_v, float max_v);

    bool resize(int width, int height);

    void clear();

    void keepAspectRatio(bool keep) { keep_aspect_ratio_ = keep; }

    [[nodiscard]] GLuint texture() const { return texture_; }
};

} // namespace recastx::gui

#endif //GUI_IMAGEBUFFER_HPP
