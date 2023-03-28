#include "graphics/items/graphics_item.hpp"
#include "graphics/scene.hpp"

namespace recastx::gui {

// class GraphicsItem

GraphicsItem::GraphicsItem(Scene& scene) : scene_(scene) {}

GraphicsItem::~GraphicsItem() = default;

void GraphicsItem::init() {}

void GraphicsItem::onWindowSizeChanged(int /*width*/, int /*height*/) {}

} // namespace recastx::gui