#ifndef GUI_GLYPHRENDERER_H
#define GUI_GLYPHRENDERER_H

#include <map>
#include <string>

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

        Character(GLuint texture_id,
                  const glm::ivec2 &size,
                  const glm::ivec2 &bearing,
                  GLuint advance)
                : texture_id(texture_id),
                  size(size),
                  bearing(bearing),
                  advance(advance) {}

        ~Character() {
            glDeleteTextures(1, &texture_id);
        }

        Character(const Character &) = delete;

        Character &operator=(const Character &) = delete;

        Character(Character &&other) noexcept {
            texture_id = other.texture_id;
            size = other.size;
            bearing = other.bearing;
            advance = other.advance;
            other.texture_id = 0;
        }

        Character &operator=(Character &&other) noexcept {
            texture_id = other.texture_id;
            size = other.size;
            bearing = other.bearing;
            advance = other.advance;
            other.texture_id = 0;
            return *this;
        }

    };

private:

    GLuint vao_;
    GLuint vbo_;

    std::map<unsigned char, Character> characters_;

public:

    GlyphRenderer();

    ~GlyphRenderer();

    void init(unsigned int pixel_width = 0, unsigned int pixel_height = 24);

    void render(const std::string &text, float x, float y, float sx, float sy);
};

}  // namespace recastx::gui

#endif //GUI_GLYPHRENDERER_H
