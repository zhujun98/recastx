/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>
#include <iostream>

#include "graphics/projection.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/image_buffer.hpp"

namespace recastx::gui {

Projection::Projection(uint32_t proj_id)
        : id_(proj_id), buffer_(new ImageBuffer), cm_(new Colormap) {
    buffer_->keepAspectRatio(true);

    cm_->set(ImPlotColormap_Greys);
}

Projection::~Projection() = default;

void Projection::setData(const DataType& data, const std::array<size_t, 2>& size) {
    texture_.setData(data, size[0], size[1]);

    cm_->bind();
    texture_.bind();
    buffer_->render(size[0], size[1]);
    texture_.unbind();
    cm_->unbind();
}

void Projection::resize(int width, int height) {
    buffer_->resize(width, height);
}

GLuint Projection::texture() const { return buffer_->texture(); }

} // namespace recastx::gui