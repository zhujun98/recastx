#include "graphics/items/graphics_item.hpp"

namespace tomcat::gui {

// class GraphicsItem

GraphicsItem::GraphicsItem(GraphicsItem::ComponentType type, Scene& scene)
    : type_(type), scene_(scene) {}

GraphicsItem::~GraphicsItem() = default;

void GraphicsItem::init() {}

void GraphicsItem::onWindowSizeChanged(int width, int height) {}

// class StaticGraphicsItem

StaticGraphicsItem::StaticGraphicsItem(Scene &scene)
        : GraphicsItem(GraphicsItem::ComponentType::STATIC, scene) {}

StaticGraphicsItem::~StaticGraphicsItem() = default;

// class DynamicGraphicsItem

DynamicGraphicsItem::DynamicGraphicsItem(Scene &scene)
    : GraphicsItem(GraphicsItem::ComponentType::DYNAMIC, scene) {}

DynamicGraphicsItem::~DynamicGraphicsItem() = default;

} // namespace tomcat::gui