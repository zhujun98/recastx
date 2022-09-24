#ifndef GUI_AXES_ITEM_H
#define GUI_AXES_ITEM_H

#include <memory>

#include "graphics/items/graphics_item.hpp"
#include "graphics/items/scene.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class AxesItem : public StaticGraphicsItem {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    bool visible_ = true;

public:

    explicit AxesItem(Scene& scene);

    ~AxesItem() override;

    void renderIm() override;

    void renderGl() override;
};

}  // namespace tomcat::gui

#endif // GUI_AXES_ITEM_H