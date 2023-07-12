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

#include "graphics/items/icon_item.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/glyph_renderer.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

IconItem::IconItem(Scene &scene)
        : GraphicsItem(scene),
          color1_(glm::vec3(0.7f, 0.8f, 0.2f)),
          color2_(glm::vec3(0.5f, 0.8f, 0.4f)),
          view_(glm::mat4(1.0f)),
          translate_(glm::translate(glm::vec3(-1.0f, -0.7f, 0.f))) {
    scene.addItem(this);

    auto glyph_vert =
#include "../shaders/glyph.vert"
    ;
    auto glyph_frag =
#include "../shaders/glyph.frag"
    ;

    glyph_shader_ = std::make_unique<ShaderProgram>(glyph_vert, glyph_frag);

    glyph_renderer_ = std::make_unique<GlyphRenderer>();
    glyph_renderer_->init(font_size_, font_size_);
}

IconItem::~IconItem() = default;

void IconItem::renderIm() {}

void IconItem::renderGl(const glm::mat4& /*view*/,
                        const glm::mat4& projection,
                        const RenderParams& params) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto asp = std::any_cast<float>(params.at("aspectRatio"));
    glyph_shader_->use();
    glyph_shader_->setInt("glyphTexture", 1);
    glyph_shader_->setMat4("view", view_);
    glyph_shader_->setMat4("projection", translate_ * projection);
    glyph_shader_->setVec3("glyphColor", color1_);
    glyph_renderer_->render("RECAST-X", 0.f, 0.55f, 0.005f * asp, 0.02f);
    glyph_shader_->setVec3("glyphColor", color2_);
    glyph_renderer_->render("@TOMCAT", 0.f, -0.15f, 0.0025f * asp, 0.01f);

    glDisable(GL_BLEND);
}

} // namespace recastx::gui
