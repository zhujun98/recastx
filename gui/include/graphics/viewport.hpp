#ifndef GUI_VIEWPORT_HPP
#define GUI_VIEWPORT_HPP

#include <glm/glm.hpp>

namespace tomcat::gui {

class Viewport {

protected:

    int x_ = 0;
    int y_ = 0;
    int w_ = 1;
    int h_ = 1;

    glm::mat4 projection_;

public:

    Viewport();

    virtual ~Viewport();

    void update(int x, int y, int w, int h);

    [[nodiscard]] const glm::mat4& projection() const { return projection_; }

    void use() const;
};

} // namespace tomcat::gui

#endif //GUI_VIEWPORT_HPP