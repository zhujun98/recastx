#ifndef GUI_AXISCUBE_ITEM_H
#define GUI_AXISCUBE_ITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class GlyphRenderer;
class Scene;

class AxiscubeItem : public GraphicsItem {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    std::unique_ptr<ShaderProgram> glyph_shader_;
    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    glm::vec3 text_color_;
    glm::mat4 top_;

public:

    explicit AxiscubeItem(Scene& scene);

    ~AxiscubeItem() override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;
};

}  // namespace tomcat::gui

#endif // GUI_AXISCUBE_ITEM_H