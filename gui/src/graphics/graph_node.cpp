/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/graph_node.hpp"

namespace recastx::gui {

GraphNode::GraphNode() : parent_(nullptr) {};

GraphNode::~GraphNode() {
    for (auto child : children_) delete child;
}

GraphNode* GraphNode::parent() const { return parent_; }

void GraphNode::appendChildNode(GraphNode* node) { children_.push_back(node); }

} // namespace recastx::gui
