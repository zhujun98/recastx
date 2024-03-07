/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>

#include "graphics/iso_surface.hpp"
#include "graphics/marchers.hpp"

namespace recastx::gui {

IsoSurface::IsoSurface() : dx_(1), dy_(1), dz_(1), value_(0.f) {
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
}

IsoSurface::~IsoSurface() {
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}

void IsoSurface::setVoxelSize(int dx, int dy, int dz) {
    dx_ = dx;
    dy_ = dy;
    dz_ = dz;
    reset();
}

void IsoSurface::reset() {
    vertices_.clear();
    initialized_ = false;
}

void IsoSurface::setValue(float v) {
    value_ = v;
    reset();
}

void IsoSurface::polygonize(const DataType& data, uint32_t x, uint32_t y, uint32_t z) {
    if (!initialized_) {
        Marcher marcher(data, x, y, z, value_);
        vertices_ = marcher.march(dx_, dy_, dz_);
        initBufferData();
        initialized_ = true;
    }
}

void IsoSurface::draw() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, vertices_.size());

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void IsoSurface::initBufferData() {
    using Vertex = SurfaceVertices::value_type;

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData (GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, normal));
}

} // namespace recastx::gui