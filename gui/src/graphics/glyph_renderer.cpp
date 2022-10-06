#include <filesystem>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics/glyph_renderer.hpp"

namespace tomcat::gui {

GlyphRenderer::GlyphRenderer() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GlyphRenderer::~GlyphRenderer() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void GlyphRenderer::init(unsigned int pixel_width, unsigned int pixel_height) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("FREETYPE: Could not init FreeType Library");
    }

    std::string font_name = std::filesystem::path("/Library/Fonts/Arial Unicode.ttf");
    if (font_name.empty()) {
        throw std::runtime_error("FREETYPE: Failed to load font_name");
    }

    // load font as face
    FT_Face face;
    int error = FT_New_Face(ft, font_name.c_str(), 0, &face);
    if (error) {
        throw std::runtime_error("FREETYPE: Failed to load font");
    } else {
        FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                throw std::runtime_error("FREETYPE: Failed to load Glyph");
            }
            GLuint texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            Character character{
                    texture_id,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<unsigned int>(face->glyph->advance.x)
            };
            characters_.insert(std::make_pair(c, std::move(character)));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void GlyphRenderer::render(const std::string& text, float x, float y, float sx, float sy) {
    glActiveTexture(GL_TEXTURE1);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // iterate through all characters
    for (unsigned char c : text) {
        const Character& ch = characters_.at(c);

        float xpos = x + ch.bearing.x * sx;
        float ypos = y - (ch.size.y - ch.bearing.y) * sx;

        float w = ch.size.x * sx;
        float h = ch.size.y * sy;
        // update VBO for each character
        float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.advance >> 6) * sx;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace tomcat::gui
