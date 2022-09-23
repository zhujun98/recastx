#ifndef GUI_SCENE_CAMERA3D_H
#define GUI_SCENE_CAMERA3D_H

#include <map>
#include <memory>
#include <optional>
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
    glm::vec3 center_;
    glm::vec3 up_;
    glm::vec3 right_;

    glm::mat4 rotation_;
    std::optional<glm::mat4> view_ {std::nullopt};

    float mouse_scroll_sensitivity_ = 0.1f;
    float mouse_move_sensitivity_ = 3.f;
    float key_sensitivity_ = 0.2f;

    float prev_x_ = -1.1f;
    float prev_y_ = -1.1f;

    bool dragging_ = false;
    bool fixed_ = false;

    void setPerspectiveView();

    void adjustPitch(float offset);

    void adjustYaw(float offset);

  public:

    Camera();

    ~Camera() override;

    [[nodiscard]] const glm::mat4& matrix();

    bool handleMouseButton(int button, int action) override;

    bool handleScroll(float offset) override;

    bool handleMouseMoved(float x, float y) override;

    bool handleKey(int key, int action, int mods) override;

    void renderIm() override;
};

} // namespace tomcat::gui

#endif // GUI_SCENE_CAMERA3D_H