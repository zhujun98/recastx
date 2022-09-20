#ifndef GUI_AXES_COMPONENT_H
#define GUI_AXES_COMPONENT_H

#include <memory>

#include "./scene_component.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class AxesComponent : public StaticSceneComponent {

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    bool visible_ = true;

public:

    explicit AxesComponent(Scene& scene);

    ~AxesComponent() override;

    void renderIm(int width, int height) override;

    void renderGl(const glm::mat4& world_to_screen) override;
};

}  // namespace tomcat::gui

#endif // GUI_AXES_COMPONENT_H