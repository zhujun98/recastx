/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_IMAGEBUFFER_HPP
#define GUI_IMAGEBUFFER_HPP

#include <vector>

#include <imgui.h>
#include "GL/gl3w.h"

#include "style.hpp"

namespace recastx::gui {

class ShaderProgram;

class ImageBuffer {

    GLuint vao_;
    GLuint vbo_;

    int width_ {8};
    int height_ {8};
    ImVec4 background_ { Style::IMAGE_BUFFER_BG_COLOR };

    GLuint fbo_;
    GLuint texture_;
    GLuint rbo_;

    bool keep_aspect_ratio_ { true };

    std::unique_ptr<ShaderProgram> shader_;

    void updateBuffer() const;

    std::array<int, 2> computeImageSize(int width, int height);

  public:

    ImageBuffer();
    ~ImageBuffer();

    void render(int width, int height);

    void keepAspectRatio(bool keep) { keep_aspect_ratio_ = keep; }

    void resize(int width, int height);

    GLuint texture() const { return texture_; }
};

} // namespace recastx::gui

#endif //GUI_IMAGEBUFFER_HPP
