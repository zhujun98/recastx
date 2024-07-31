/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <filesystem>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics/glyph_renderer.hpp"

namespace recastx::gui {

GlyphRenderer::GlyphRenderer() = default;

GlyphRenderer::~GlyphRenderer() = default;

void GlyphRenderer::init(unsigned int pixel_height) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("FREETYPE: Could not init FreeType Library");
    }

#if defined(__APPLE__)
    std::string font_name = std::filesystem::path("/Library/Fonts/Arial Unicode.ttf");
#elif defined(_WIN32)
    std::string font_name = std::filesystem::path("C:\\Windows/Fonts/arial.ttf").string();
#else
    std::string font_name = std::filesystem::path("/usr/share/fonts/truetype/freefont/FreeMono.ttf");
#endif
    if (font_name.empty()) {
        throw std::runtime_error("FREETYPE: Failed to load font_name");
    }

    FT_Face face;
    int error = FT_New_Face(ft, font_name.c_str(), 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        throw std::runtime_error("FREETYPE: unsupported font format");
    } else if (error) {
        throw std::runtime_error("FREETYPE: Failed to load font");
    } else {
        FT_Set_Pixel_Sizes(face, 0, pixel_height);
        y_scale_ = 1.f / static_cast<float>(pixel_height);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                throw std::runtime_error("FREETYPE: Failed to load Glyph");
            }
            // generate texture
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

const GlyphRenderer::Character& GlyphRenderer::getCharacter(char c) const {
    return characters_.at(c);
}

}  // namespace recastx::gui
