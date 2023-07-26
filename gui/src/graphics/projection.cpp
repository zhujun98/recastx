/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/projection.hpp"

namespace recastx::gui {

Projection::Projection(int proj_id) : id_(proj_id) {}

Projection::~Projection() = default;

int Projection::id() const { return id_; }

void Projection::setData(DataType&& data, const SizeType& size) {
    data_ = std::move(data);
    size_ = size;

    texture_.setData(data_, static_cast<int>(size_[0]), static_cast<int>(size_[1]));
}

} // namespace recastx::gui