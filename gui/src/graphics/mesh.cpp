/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cstring>
#include <utility>

#include "graphics/mesh.hpp"

namespace recastx::gui {

Mesh::Mesh(const float vertices[], int num_vertices) : vertices_(num_vertices) {
    std::memcpy(vertices_.data(), vertices, num_vertices * 8 * sizeof(float));
    init();
}

Mesh::Mesh(std::vector<Vertex> &&vertices) {
    vertices_ = std::move(vertices);
    init();
}


Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<Index> &indices) :
        vertices_(vertices), indices_(indices) {
    init();
}

Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<Index> &indices,
           unsigned int material_index,
           const char *name) :
        vertices_(vertices), indices_(indices), material_index_(material_index), name_(name) {
    init();
}

Mesh::~Mesh() {
    if (!indices_.empty()) glDeleteBuffers(1, &EBO_);
    glDeleteBuffers(1, &VBO_);
    glDeleteVertexArrays(1, &VAO_);
}

Mesh::Mesh(Mesh &&other) noexcept
        : vertices_(std::move(other.vertices_)),
          indices_(std::move(other.indices_)),
          material_index_(std::exchange(other.material_index_, 0)),
          name_(std::move(other.name_)),
          VAO_(std::exchange(other.VAO_, 0)),
          VBO_(std::exchange(other.VBO_, 0)),
          EBO_(std::exchange(other.EBO_, 0)) {
}

void Mesh::render() const {
    glBindVertexArray(VAO_);
    if (!indices_.empty()) {
        glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
    }
    glBindVertexArray(0);
}

void Mesh::init() {
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    if (!indices_.empty()) glGenBuffers(1, &EBO_);

    glBindVertexArray(VAO_);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);

    if (!indices_.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(Index), &indices_[0], GL_STATIC_DRAW);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tex));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));

    glBindVertexArray(0);
}

} // recastx::gui