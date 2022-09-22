#ifndef GUI_GRAPHNODE_H
#define GUI_GRAPHNODE_H

#include <list>

#include <glm/glm.hpp>

namespace tomcat::gui {

class GraphNode {

    GraphNode *parent_;

    std::list<GraphNode*> children_;

public:

    GraphNode();

    virtual ~GraphNode();

    [[nodiscard]] GraphNode *parent() const;

    void appendChildNode(GraphNode* node);

    virtual void renderIm() = 0;

};


class GraphGlNode : public GraphNode {

public:

    virtual void renderGl() = 0;

};

} // namespace tomcat::gui

#endif //GUI_GRAPHNODE_H