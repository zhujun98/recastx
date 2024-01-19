/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/graphics_item.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

// class GraphicsItem

GraphicsItem::GraphicsItem(Scene& scene) : scene_(scene) {}

GraphicsItem::~GraphicsItem() = default;

} // namespace recastx::gui