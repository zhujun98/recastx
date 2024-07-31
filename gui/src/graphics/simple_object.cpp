/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/simple_object.hpp"
#include "graphics/renderer.hpp"

namespace recastx::gui {

namespace details {

SimpleObject::Model genCubeFrameVertices() {
    std::vector<float> vertices {
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f
    };

    std::vector<unsigned> indices {
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
    };

    return {8, vertices, indices, SimpleObject::Mode::LINE };
}

SimpleObject::Model genAxisCubeVertices() {
    std::vector<float> vertices {
            // back face
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,

            -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

            -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,

            0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,

            -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
    };

    std::vector<unsigned int> indices {
            0, 1, 2, 1, 0, 3,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 13, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 21, 23, 20
    };

    return { 24, vertices, indices, SimpleObject::Mode::SURFACE };
};

SimpleObject::Model genAxisVertices() {
    std::vector<float> vertices{
            0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            1.f, 0.f, 0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
            0.f, 1.f, 0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 0.f, 0.f, 1.f,
            0.f, 0.f, 1.f, 0.f, 0.f, 1.f
    };

    std::vector<unsigned> indices {
            0, 1, 2, 3, 4, 5
    };

    return {6, vertices, indices, SimpleObject::Mode::LINE };
}

SimpleObject::Model genSquareVertices() {
    std::vector<float> vertices{
            -0.5f, -0.5f, 0.f,
             0.5f,  0.5f, 0.f,
            -0.5f,  0.5f, 0.f,
             0.5f, -0.5f, 0.f
    };

    std::vector<unsigned> indices {
            0, 1, 2, 0, 3, 1
    };

    return {4, vertices, indices, SimpleObject::Mode::SURFACE };
}

} // details

SimpleObject::SimpleObject() {
    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);
}

SimpleObject::SimpleObject(SimpleObject::Type type) {
    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);

    if (type == Type::CUBE_FRAME) {
        model_ = details::genCubeFrameVertices();
    } else if (type == Type::AXIS) {
        model_ = details::genAxisVertices();
    } else if (type == Type::AXIS_CUBE) {
        model_ = details::genAxisCubeVertices();
    } else if (type == Type::SQUARE) {
        model_ = details::genSquareVertices();
    } else {
        throw std::runtime_error("Unknown shape");
    }

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);

    init();
}

SimpleObject::~SimpleObject() {
    glDeleteBuffers(1, &EBO_);
    glDeleteBuffers(1, &VBO_);
    glDeleteVertexArrays(1, &VAO_);
}

void SimpleObject::init() {
    const auto& [num_indices, vertices, indices, mode] = model_;

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    int vertex_size = vertices.size() / num_indices;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    if (vertex_size == 6) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
}

void SimpleObject::render(Renderer* renderer) {
    shader_->use();
    shader_->setMat4("mvp", renderer->vpMatrix() * model());
    shader_->setBool("useVertexColor", model_.vertices.size() / model_.num_vertices == 6);
    shader_->setVec3("color", color_);

    glBindVertexArray(VAO_);
    if (model_.mode == Mode::SURFACE) {
        glDrawElements(GL_TRIANGLES, model_.indices.size(), GL_UNSIGNED_INT, 0);
    } else {
#ifndef __APPLE__
        glLineWidth(0.5f);
#endif
        glDrawElements(GL_LINES, model_.indices.size(), GL_UNSIGNED_INT, 0);
    }
}

const char* SimpleObject::vertex_shader = R"glsl(
#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;

out vec3 vertexColor;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(vPos, 1.f);
    vertexColor = vColor;
}
)glsl";

const char* SimpleObject::fragment_shader = R"glsl(
#version 330 core

in vec3 vertexColor;

out vec4 fColor;

uniform bool useVertexColor;
uniform vec3 color;

void main() {
    if (useVertexColor) {
        fColor = vec4(color * vertexColor, 1.0);
    } else {
        fColor = vec4(color, 1.0);
    }
}
)glsl";

} // namespace recastx::gui