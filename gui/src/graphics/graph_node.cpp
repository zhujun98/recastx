#include "graphics/graph_node.hpp"

namespace tomcat::gui {

GraphNode::GraphNode() : parent_(nullptr) {};

GraphNode::~GraphNode() {
    for (auto child : children_) delete child;
}

GraphNode* GraphNode::parent() const { return parent_; }

void GraphNode::appendChildNode(GraphNode* node) { children_.push_back(node); }

} // namespace tomcat::gui
