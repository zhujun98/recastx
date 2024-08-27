/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/image_object.hpp"
#include "graphics/renderer.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/style.hpp"

namespace recastx::gui {

ImageObject::ImageObject() {

    auto vertex_shader =
#include "shaders/image.vert"
    ;
    auto fragment_shader =
#include "shaders/image.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);
    shader_->use();
    shader_->setInt("imageTexture", 0);
    shader_->setInt("lutColor", 1);
    shader_->setVec4("frameColor", Style::K_EMPTY_FRAME_COLOR);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    init();
}

ImageObject::~ImageObject() {
    glDeleteBuffers(1, &VBO_);
    glDeleteVertexArrays(1, &VAO_);
}

void ImageObject::init() {
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
}

void ImageObject::render(Renderer* renderer) {
    static constexpr float scale = 0.98f;
    if (keep_aspect_ratio_) {
        float ratio = renderer->aspectRatio() * aspect_ratio_;
        if (ratio >= 1) {
            setScale({scale / ratio, scale, 1.f});
        } else {
            setScale({scale, ratio, 1.f});
        }
    } else {
        setScale({scale, scale, 1.f});
    }

    shader_->use();

    auto mat = MaterialManager::instance().getMaterial<TransferFunc>(mat_id_);
    auto [min_v, max_v] = mat->minMaxVals();
    shader_->setMat4("model", model());
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    shader_->setBool("drawFrame", !intensity_.initialized());

    intensity_.bind(0);
    mat->bindColor(1);

    glBindVertexArray(VAO_);
    if (intensity_.initialized()) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else {
#ifndef __APPLE__
        glLineWidth(2.f);
#endif
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    mat->unbindColor();
    intensity_.unbind();
}

void ImageObject::renderGUI() {
    ImGui::Checkbox("Keep Aspect Ratio##PROJ_COMP", &keep_aspect_ratio_);
}

} // recastx::gui
