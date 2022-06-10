#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "shader_program.hpp"

#include "object_component.hpp"

namespace gui {

class SceneObject;

class AxesComponent : public ObjectComponent {

    SceneObject& object_;

    GLuint axes_vao_handle_;
    GLuint axes_vbo_handle_;
    GLuint axes_index_handle_;
    int axes_index_count_;
    std::unique_ptr<ShaderProgram> axes_program_;

    bool show_ = true;

   public:

    explicit AxesComponent(SceneObject& object);

    void draw(glm::mat4 world_to_screen) override;

    [[nodiscard]] std::string identifier() const override { return "axes"; }

    void describe() override;
};

}  // namespace gui
