#ifndef GUI_AXESITEM_H
#define GUI_AXESITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

class Scene;

class AxesItem : public GraphicsItem, public GraphicsGLItem {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    bool visible_ = true;

public:

    explicit AxesItem(Scene& scene);

    ~AxesItem() override;

    void renderIm() override;

    void renderGl(const glm::mat4& view,
                  const glm::mat4& projection,
                  const RenderParams& params) override;
};

}  // namespace recastx::gui

#endif // GUI_AXESITEM_H