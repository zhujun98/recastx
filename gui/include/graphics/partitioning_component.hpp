#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "scene_object.hpp"
#include "shader_program.hpp"
#include "object_component.hpp"

namespace gui {

struct part {
    part(int id_) : id(id_) {}
    part(int id_, glm::vec3 min_pt_, glm::vec3 max_pt_)
        : id(id_), min_pt(min_pt_), max_pt(max_pt_) {}

    int id;
    glm::vec3 min_pt = {0.0f, 0.0f, 0.0f};
    glm::vec3 max_pt = {1.0f, 1.0f, 1.0f};
};

class PartitioningComponent : public ObjectComponent {

    SceneObject& object_;

    GLuint cube_vao_handle_;
    GLuint cube_vbo_handle_;
    std::unique_ptr<ShaderProgram> part_program_;

    std::vector<part> parts_;
    bool show_ = true;

    float global_scale_ = 1.0f;

  public:

    explicit PartitioningComponent(SceneObject& object);
    ~PartitioningComponent();

    void draw(glm::mat4 world_to_screen) override;

    [[nodiscard]] std::string identifier() const override { return "partitioning"; }

    void add_part(part&& p) { parts_.push_back(std::move(p)); }
    void describe() override;
};

}  // namespace gui
