#ifndef GUI_SCENE_CAMERA3D_H
#define GUI_SCENE_CAMERA3D_H

#include <map>
#include <memory>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "input_handler.hpp"
#include "graphics/graph_node.hpp"
#include "path.hpp"
#include "ticker.hpp"

namespace tomcat::gui {

class Camera : public GraphNode, public InputHandler {

    glm::vec3 pos_;
    glm::vec3 center_ {0.f, 0.f, 0.f};
    glm::vec3 up_;
    glm::vec3 right_;

    float mouse_sensitivity_ = 3.0f;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;
    glm::vec2 delta_;

    bool dragging_ = false;
    bool fixed_ = false;

    glm::mat4 rotation_;

    void setPerspectiveView();

  public:

    Camera();

    ~Camera() override;

    glm::mat4 matrix();

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(double offset) override;

    bool handleMouseMoved(double x, double y) override;

    bool handleKey(int key, int action, int mods) override;

    void rotate(float phi, float psi);

    void renderIm() override;
};

} // namespace tomcat::gui

#endif // GUI_SCENE_CAMERA3D_H