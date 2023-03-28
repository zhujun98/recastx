#ifndef GUI_GRAPHNODE_H
#define GUI_GRAPHNODE_H

#include <list>

#include <glm/glm.hpp>

namespace recastx::gui {

class GraphNode {

    GraphNode *parent_;

    std::list<GraphNode*> children_;

public:

    GraphNode();

    virtual ~GraphNode();

    [[nodiscard]] GraphNode *parent() const;

    void appendChildNode(GraphNode* node);
};

} // namespace recastx::gui

#endif //GUI_GRAPHNODE_H