/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics/items/axiscube_item.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/glyph_renderer.hpp"
#include "graphics/primitives.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

AxiscubeItem::AxiscubeItem(Scene &scene)
        : GraphicsItem(scene),
          text_color_(glm::vec3(1.f, 1.f, 1.f)),
          top_(glm::translate(glm::vec3(-0.2f, 0.2f, 0.501f))
               * glm::rotate(glm::radians(-90.f), glm::vec3(0.0f, .0f, 1.0f))){
    scene.addItem(this);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::axiscube), primitives::axiscube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "../shaders/simple_cube.vert"
    ;
    auto frag =
#include "../shaders/simple_cube.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    auto glyph_vert =
#include "../shaders/glyph.vert"
    ;
    auto glyph_frag =
#include "../shaders/glyph.frag"
    ;

    glyph_shader_ = std::make_unique<ShaderProgram>(glyph_vert, glyph_frag);

    glyph_renderer_ = std::make_unique<GlyphRenderer>();
    glyph_renderer_->init(24, 24);
}

AxiscubeItem::~AxiscubeItem() = default;

void AxiscubeItem::renderIm() {}

void AxiscubeItem::renderGl(const glm::mat4& view,
                            const glm::mat4& projection,
                            const RenderParams& /*params*/) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_->use();
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glyph_shader_->use();
    glyph_shader_->setMat4("projection", projection);
    glyph_shader_->setVec3("glyphColor", text_color_);
    glyph_shader_->setInt("glyphTexture", 1);
    // top
    glyph_shader_->setMat4("view", view * top_);
    glyph_renderer_->render("Top", 0.f, 0.f, 0.01f, 0.02f);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

} // namespace recastx::gui
