/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/items/graphics_item.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

// class GraphicsItem

GraphicsItem::GraphicsItem(Scene& scene) : scene_(scene) {}

GraphicsItem::~GraphicsItem() = default;

void GraphicsItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

} // namespace recastx::gui