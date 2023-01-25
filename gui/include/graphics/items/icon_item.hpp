#ifndef GUI_ICONITEM_H
#define GUI_ICONITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class GlyphRenderer;
class Scene;

class IconItem : public GraphicsItem {

    std::unique_ptr<ShaderProgram> glyph_shader_;
    std::unique_ptr<GlyphRenderer> glyph_renderer_;

    glm::vec3 color_;
    glm::mat4 view_;
    glm::mat4 translate_;

public:

    explicit IconItem(Scene& scene);

    ~IconItem() override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;
};

}  // namespace tomcat::gui

#endif // GUI_ICONITEM_H