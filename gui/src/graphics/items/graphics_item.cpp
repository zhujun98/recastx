#include "graphics/items/graphics_item.hpp"
#include "graphics/scene.hpp"

namespace tomcat::gui {

// class GraphicsItem

GraphicsItem::GraphicsItem(Scene& scene) : scene_(scene) {
    scene_.addItem(this);
}

GraphicsItem::~GraphicsItem() = default;

void GraphicsItem::init() {}

void GraphicsItem::onWindowSizeChanged(int width, int height) {}

// class GraphicsDataItem

GraphicsDataItem::GraphicsDataItem(Scene &scene)
    : GraphicsItem(scene) {}

GraphicsDataItem::~GraphicsDataItem() = default;

} // namespace tomcat::gui