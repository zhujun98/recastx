#ifndef GUI_AXES_COMPONENT_H
#define GUI_AXES_COMPONENT_H

#include <memory>

#include "./scene_component.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class AxesComponent : public StaticSceneComponent {

    GLuint axes_vao_handle_;
    GLuint axes_vbo_handle_;
    GLuint axes_index_handle_;
    int axes_index_count_;
    std::unique_ptr<ShaderProgram> axes_program_;

    bool show_ = true;

public:

    explicit AxesComponent(Scene& scene);

    ~AxesComponent() override;

    void renderIm() override;

    void renderGl(const glm::mat4& world_to_screen) override;
};

}  // namespace tomcat::gui

#endif // GUI_AXES_COMPONENT_H