#ifndef GUI_AXES_COMPONENT_H
#define GUI_AXES_COMPONENT_H

#include <memory>

#include "graphics/components/scene_component.hpp"
#include "graphics/components/scene.hpp"
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

    void renderIm() override;

    void renderGl() override;
};

}  // namespace tomcat::gui

#endif // GUI_AXES_COMPONENT_H