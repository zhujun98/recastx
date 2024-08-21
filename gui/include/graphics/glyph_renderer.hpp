/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_GLYPHRENDERER_H
#define GUI_GLYPHRENDERER_H

#include <unordered_map>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

namespace recastx::gui {

class GlyphRenderer {

  public:

    struct Character {
        GLuint texture_id;
        glm::ivec2 size;
        glm::ivec2 bearing;
        GLuint advance;

        Character(GLuint texture_id, const glm::ivec2& size, const glm::ivec2& bearing, GLuint advance)
                : texture_id(texture_id), size(size), bearing(bearing), advance(advance) {}

        ~Character() {
            if (texture_id > 0) glDeleteTextures(1, &texture_id);
        }

        Character(const Character&) = delete;
        Character& operator=(const Character&) = delete;

        Character(Character&& other) noexcept {
            texture_id = other.texture_id;
            size = other.size;
            bearing = other.bearing;
            advance = other.advance;
            other.texture_id = 0;
        }

        Character& operator=(Character&& other) noexcept {
            texture_id = other.texture_id;
            size = other.size;
            bearing = other.bearing;
            advance = other.advance;
            other.texture_id = 0;
            return *this;
        }

    };

  private:

    std::unordered_map<unsigned char, Character> characters_;

    float y_scale_ = 1.f;

  public:

    GlyphRenderer();

    ~GlyphRenderer();

    void init(unsigned int pixel_height);

    [[nodiscard]] const Character& getCharacter(char c) const;

    [[nodiscard]] float yScale() const { return y_scale_; }
};

}  // namespace recastx::gui

#endif //GUI_GLYPHRENDERER_H
