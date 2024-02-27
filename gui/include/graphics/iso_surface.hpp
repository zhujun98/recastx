/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_ISOSURFACE_H
#define GUI_ISOSURFACE_H

#include <vector>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include "iso_surface_interface.hpp"

namespace recastx::gui {

struct Light;

class IsoSurface {

  public:

    using DataType = std::vector<float>;

  private:

    GLuint VAO_;
    GLuint VBO_;

    int dx_;
    int dy_;
    int dz_;

    float value_;
    bool initialized_ = false;
    SurfaceVertices vertices_;

    void initBufferData();

  public:

    IsoSurface();

    ~IsoSurface();

    void setVoxelSize(int dx, int dy, int dz);

    void reset();

    void setValue(float v);

    void polygonize(const DataType& data, uint32_t x, uint32_t y, uint32_t z);

    void draw();
};

} // namespace recastx::gui

#endif // GUI_ISOSURFACE_H
