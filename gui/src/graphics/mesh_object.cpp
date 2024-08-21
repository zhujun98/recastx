/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/mesh_object.hpp"
#include "graphics/renderer.hpp"
#include "graphics/mesh.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/widgets/widget.hpp"

namespace recastx::gui {

namespace details {

// positions (3) texture coords (2) normals (3)
static constexpr float cube[] = {
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
         0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
         0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
         0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
         0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f
};

}

MeshObject::MeshObject(ShapeType shape) {

    auto vertex_shader =
#include "shaders/mesh.vert"
    ;
    auto fragment_shader =
#include "shaders/mesh.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    if (shape == ShapeType::CUBE) {
        mesh_ = std::make_unique<Mesh>(details::cube, 36);
    } else {
        throw std::runtime_error("Unknown shape");
    }
}

MeshObject::MeshObject(std::vector<Vertex> &&vertices) {

    auto vertex_shader =
#include "shaders/mesh.vert"
    ;
    auto fragment_shader =
#include "shaders/mesh.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    mesh_ = std::make_unique<Mesh>(std::move(vertices));;
}

MeshObject::~MeshObject() = default;

void MeshObject::setVertices(std::vector<Vertex> &&vertices) {
    mesh_ = std::make_unique<Mesh>(std::move(vertices));
}

void MeshObject::render(Renderer *renderer) {
    shader_->use();

    shader_->setMat4("model", model());
    shader_->setMat4("mvp", renderer->vpMatrix() * model());
    shader_->setVec3("viewPos", renderer->viewPos());

    auto light = renderer->light();
    shader_->setVec3("light.position", light->position());
    shader_->setVec3("light.ambient", light->ambient());
    shader_->setVec3("light.diffuse", light->diffuse());
    shader_->setVec3("light.specular", light->specular());

    if (show_mesh_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    auto mat = MaterialManager::instance().getMaterial<MeshMaterial>(mat_id_);
    shader_->setInt("material.ambient", 0);
    shader_->setInt("material.diffuse", 1);
    shader_->setInt("material.specular", 2);
    shader_->setFloat("material.shininess", mat->shininess());
    mat->bind(0, 1, 2);
    mesh_->render();
    mat->unbind();

    if (show_mesh_) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MeshObject::renderGUI() {
    ImGui::Checkbox("Show Mesh", &show_mesh_);
}

} // recastx::gui