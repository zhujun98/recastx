#ifndef GUI_AXES_COMPONENT_H
#define GUI_AXES_COMPONENT_H

#include <memory>

#include "scene_component.hpp"
#include "graphics/shader_program.hpp"

namespace tomcat::gui {

class Scene;

class AxesComponent : public SceneComponent {

    Scene& scene_;

    GLuint axes_vao_handle_;
    GLuint axes_vbo_handle_;
    GLuint axes_index_handle_;
    int axes_index_count_;
    std::unique_ptr<ShaderProgram> axes_program_;

    bool show_ = true;

   public:

    explicit AxesComponent(Scene& scene);

    void render(const glm::mat4& world_to_screen) override;

    [[nodiscard]] std::string identifier() const override { return "axes"; }

    void describe() override;
};

}  // tomcat::gui

#endif // GUI_AXES_COMPONENT_H