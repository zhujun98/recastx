/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MESH_H
#define GUI_MESH_H

#include <string>
#include <vector>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "vertex.hpp"

namespace recastx::gui {

class Mesh {

  public:

    using Index = unsigned int;

  private:

    std::vector<Vertex> vertices_;
    std::vector<Index> indices_;
    unsigned int material_index_ = 0;
    std::string name_;

    GLuint VAO_;
    GLuint VBO_;
    GLuint EBO_;

    void init();

  public:

    Mesh(const float vertices[], int num_vertices);

    explicit Mesh(std::vector<Vertex> &&vertices);

    Mesh(const std::vector<Vertex> &vertices, const std::vector<Index> &indices);

    Mesh(const std::vector<Vertex> &vertices,
         const std::vector<Index> &indices,
         unsigned int material_index,
         const char *name);

    ~Mesh();

    Mesh(const Mesh &other) = delete;

    Mesh &operator=(const Mesh &other) = delete;

    Mesh(Mesh &&other) noexcept;

    Mesh &operator=(Mesh &&other) = delete;

    void render() const;

    [[nodiscard]] const std::string &name() const { return name_; }

    [[nodiscard]] unsigned int materialIndex() const { return material_index_; }
};

} // recastx::gui

#endif // GUI_MESH_H