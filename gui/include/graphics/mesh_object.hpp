/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MESH_OBJECT_H
#define GUI_MESH_OBJECT_H

#include <string>
#include <vector>

#include "graphics/vertex.hpp"
#include "graphics/renderable.hpp"

namespace recastx::gui {

class Mesh;

enum class ShapeType {
    CUBE
};

class MeshObject : public Renderable {

    std::unique_ptr<Mesh> mesh_;

    bool show_mesh_;

  public:

    explicit MeshObject(ShapeType shape);

    explicit MeshObject(std::vector<Vertex> &&vertices);

    ~MeshObject() override;

    void render(Renderer *renderer) override;

    void renderGUI() override;

    void setVertices(std::vector<Vertex> &&vertices);
};

} // recastx::gui

#endif // GUI_MESH_OBJECT_H