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
#include "graphics/light_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/widgets/widget.hpp"

namespace recastx::gui {

static const char *vertex_shader = R"glsl(
#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;

out vec3 fPos;
out vec2 fTexCoord;
out vec3 fNormal;

uniform mat4 model;
uniform mat4 mvp;

void main() {
    fPos = vec3(model * vec4(vPos, 1.0)); // to world space
    fNormal = mat3(transpose(inverse(model))) * vNormal;
    fTexCoord = vTexCoord;

    gl_Position = mvp * vec4(vPos, 1.0);
}
)glsl";

static const char *fragment_shader = R"glsl(
#version 330 core

out vec4 fColor;

struct Material {
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
#define MAX_NUM_LIGHTS 4
uniform Light lights[MAX_NUM_LIGHTS];
uniform int numLights;

in vec3 fPos;
in vec3 fNormal;
in vec2 fTexCoord;

uniform vec3 viewPos;
uniform Material material;

vec3 computeLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    vec3 sum = vec3(0.f, 0.f, 0.f);

    sum += light.ambient * texture(material.ambient, fTexCoord).rgb;

    float diff = max(dot(normal, lightDir), 0.0);

    sum += light.diffuse * (diff * texture(material.diffuse, fTexCoord).rgb);

    float spec = 0.f;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.f), material.shininess);

    sum += light.specular * spec * texture(material.specular, fTexCoord).rgb;

    return sum;
}

void main() {

    vec3 result = vec3(0.f, 0.f, 0.f);

    vec3 normal = normalize(fNormal);
    vec3 viewDir = normalize(viewPos - fPos);

    for (int i = 0; i < numLights; ++i) {
        result += computeLight(lights[i], normal, viewDir);
    }

    fColor = vec4(result, 1.0);
}
)glsl";

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
    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    if (shape == ShapeType::CUBE) {
        mesh_ = std::make_unique<Mesh>(details::cube, 36);
    } else {
        throw std::runtime_error("Unknown shape");
    }
}

MeshObject::MeshObject(std::vector<Vertex> &&vertices) {
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

    auto &lights = renderer->lightManager()->lights();
    shader_->setInt("numLights", lights.size());
    for (size_t i = 0; i < lights.size(); ++i) {
        auto ptr = lights[i];
        shader_->setVec3("lights[" + std::to_string(i) + "].direction", ptr->direction());
        shader_->setVec3("lights[" + std::to_string(i) + "].ambient", ptr->ambient());
        shader_->setVec3("lights[" + std::to_string(i) + "].diffuse", ptr->diffuse());
        shader_->setVec3("lights[" + std::to_string(i) + "].specular", ptr->specular());
    }

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