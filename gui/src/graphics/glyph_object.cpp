/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/glyph_object.hpp"
#include "graphics/renderer.hpp"
#include "graphics/glyph_renderer.hpp"

namespace recastx::gui {

GlyphObject::GlyphObject(std::string text)
        : Renderable(), text_(std::move(text)), color_({0.f, 0.f, 0.f}) {
    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);

    init();
}

GlyphObject::~GlyphObject() {
    glDeleteBuffers(1, &EBO_);
    glDeleteBuffers(1, &VBO_);
    glDeleteVertexArrays(1, &VAO_);
}

void GlyphObject::render(Renderer* renderer) {
    float x0 = 0.f;
    float y0 = 0.18f;
    float scale = renderer->glyph()->yScale();

    float width = x0;
    for (unsigned char c : text_) {
        const auto &ch = renderer->glyph()->getCharacter(c);
        width += (ch.advance >> 6) * scale;
    }

    shader_->use();
    shader_->setInt("glyph", 0);
    shader_->setVec3("textColor", color_);
    shader_->setMat4("mvp", glm::ortho(0.f, width, 0.f, 1.f) * model());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    for (unsigned char c : text_) {
        const auto& ch = renderer->glyph()->getCharacter(c);

        float xpos = x0 + ch.bearing.x * scale;
        float ypos = y0 + (pos_.y - (ch.size.y - ch.bearing.y)) * scale;

        float size_x = ch.size.x * scale;
        float size_y = ch.size.y * scale;

        float vertices[4][4] = {
                { xpos,          ypos + size_y, 0.0f, 0.0f },
                { xpos,          ypos,          0.0f, 1.0f },
                { xpos + size_x, ypos,          1.0f, 1.0f },
                { xpos + size_x, ypos + size_y, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        x0 += (ch.advance >> 6) * scale;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void GlyphObject::init() {
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    unsigned int indices[6] = {0, 2, 1, 2, 3, 0};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

const char* GlyphObject::vertex_shader = R"glsl(
#version 330 core

layout (location = 0) in vec4 vPosTex; // <vec2 pos, vec2 tex>

out vec2 texCoords;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vPosTex.xy, 0.0, 1.0);
    texCoords = vPosTex.zw;
}
)glsl";

const char* GlyphObject::fragment_shader = R"glsl(
#version 330 core

in vec2 texCoords;

out vec4 fColor;

uniform sampler2D glyph;
uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(glyph, texCoords).r);
    fColor = vec4(textColor, 1.0) * sampled;
}
)glsl";

} // recastx::gui