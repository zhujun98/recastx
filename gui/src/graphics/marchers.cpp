/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <array>

#include <glm/glm.hpp>

#include "graphics/marchers.hpp"

namespace recastx::gui {

Marcher::Marcher(const Marcher::DataType &data, uint32_t x, uint32_t y, uint32_t z, float iso_value)
        : data_(data), x_(x), y_(y), z_(z), iso_value_(iso_value) {
}

SurfaceVertices Marcher::march(int dx, int dy, int dz) {
    std::array<float, 8> corner_values;
    glm::vec3 edge_vertices[12];
    glm::vec3 edge_normals[12];

    SurfaceVertices vertices_;
    for (int z = 0; z < z_; z += dz) {
        for (int y = 0; y < y_; y += dy) {
            for (int x = 0; x < x_; x += dx) {
                for (size_t i = 0; i < 8; ++i) {
                    corner_values[i] = sampleVolume(x + vertex_offset[i][0] * dx,
                                                    y + vertex_offset[i][1] * dy,
                                                    z + vertex_offset[i][2] * dz);
                }

                uint8_t cube_index = 0;
                for (size_t i_vertex = 0; i_vertex < 8; ++i_vertex) {
                    if (corner_values[i_vertex] <= iso_value_) cube_index |= 1 << i_vertex;
                }
                uint16_t edge_flags = edge_table[cube_index];

                if (edge_flags == 0) continue;

                for (size_t i_edge = 0; i_edge < 12; ++i_edge) {
                    if (edge_flags & (1 << i_edge)) {
                        auto i0 = edge_connection[i_edge][0];
                        auto i1 = edge_connection[i_edge][1];
                        float offset = getOffset(corner_values[i0], corner_values[i1]);

                        edge_vertices[i_edge].x = x + (vertex_offset[i0][0] + offset * edge_direction[i_edge][0]) * dx;
                        edge_vertices[i_edge].y = y + (vertex_offset[i0][1] + offset * edge_direction[i_edge][1]) * dy;
                        edge_vertices[i_edge].z = z + (vertex_offset[i0][2] + offset * edge_direction[i_edge][2]) * dz;

                        edge_normals[i_edge] = normalAt((int)edge_vertices[i_edge].x,
                                                        (int)edge_vertices[i_edge].y,
                                                        (int)edge_vertices[i_edge].z);
                    }
                }

                for (int i_triangle = 0; i_triangle < 5; ++i_triangle) {
                    if (tri_table[cube_index][3 * i_triangle] < 0) break;

                    for (int i_corner = 0; i_corner < 3; ++i_corner) {
                        int i_vertex = tri_table[cube_index][3 * i_triangle + i_corner];
                        SurfaceVertices::value_type vertex;
                        vertex.pos = edge_vertices[i_vertex] * glm::vec3(1./x_, 1./y_, 1./z_);
                        vertex.normal = edge_normals[i_vertex];
                        vertices_.push_back(vertex);
                    }
                }
            }
        }

    }

    return vertices_;
}

} // namespace recastx::gui