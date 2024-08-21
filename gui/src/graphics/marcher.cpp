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

#include "graphics/marcher.hpp"

namespace recastx::gui {

Marcher::Marcher() : dx_(2), dy_(2), dz_(2), iso_value_(0.f), config_changed_(true) {
}

Marcher::~Marcher() = default;

std::vector<Vertex> Marcher::march(const Marcher::DataType& data,
                                   DataType::ValueType v_min,
                                   DataType::ValueType v_max) {
    float iso_value = v_min + (v_max - v_min) * iso_value_;

    std::array<uint8_t, 8> corner_values;
    glm::vec3 edge_vertices[12];
    glm::vec3 edge_normals[12];

    std::vector<Vertex> vertices;
    for (uint32_t z = 0; z < data.z(); z += dz_) {
        for (uint32_t y = 0; y < data.y(); y += dy_) {
            for (uint32_t x = 0; x < data.x(); x += dx_) {
                for (size_t i = 0; i < 8; ++i) {
                    corner_values[i] = sampleVolume(data,
                                                    x + vertex_offset[i][0] * dx_,
                                                    y + vertex_offset[i][1] * dy_,
                                                    z + vertex_offset[i][2] * dz_);
                }

                uint8_t cube_index = 0;
                for (size_t i_vertex = 0; i_vertex < 8; ++i_vertex) {
                    if (corner_values[i_vertex] <= iso_value) cube_index |= 1 << i_vertex;
                }
                uint16_t edge_flags = edge_table[cube_index];

                if (edge_flags == 0) continue;

                for (size_t i_edge = 0; i_edge < 12; ++i_edge) {
                    if (edge_flags & (1 << i_edge)) {
                        auto i0 = edge_connection[i_edge][0];
                        auto i1 = edge_connection[i_edge][1];
                        float offset = getOffset(iso_value, corner_values[i0], corner_values[i1]);

                        edge_vertices[i_edge].x = x + (vertex_offset[i0][0] + offset * edge_direction[i_edge][0]) * dx_;
                        edge_vertices[i_edge].y = y + (vertex_offset[i0][1] + offset * edge_direction[i_edge][1]) * dy_;
                        edge_vertices[i_edge].z = z + (vertex_offset[i0][2] + offset * edge_direction[i_edge][2]) * dz_;

                        edge_normals[i_edge] = normalAt(data,
                                                        (int)edge_vertices[i_edge].x,
                                                        (int)edge_vertices[i_edge].y,
                                                        (int)edge_vertices[i_edge].z);
                    }
                }

                for (int i_triangle = 0; i_triangle < 5; ++i_triangle) {
                    if (tri_table[cube_index][3 * i_triangle] < 0) break;

                    for (int i_corner = 0; i_corner < 3; ++i_corner) {
                        int i_vertex = tri_table[cube_index][3 * i_triangle + i_corner];
                        Vertex vertex;
                        vertex.pos = edge_vertices[i_vertex] * glm::vec3(1. / data.x(), 1. / data.y(), 1. / data.z());
                        vertex.pos -= 0.5f;
                        vertex.normal = edge_normals[i_vertex];
                        vertices.push_back(vertex);
                    }
                }
            }
        }
    }

    config_changed_ = false;
    return vertices;
}

} // namespace recastx::gui