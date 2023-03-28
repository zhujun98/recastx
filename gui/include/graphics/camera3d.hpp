#ifndef GUI_CAMERA3D_H
#define GUI_CAMERA3D_H

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "path.hpp"
#include "ticker.hpp"

namespace recastx::gui {

class Camera : public InputHandler {

    glm::vec3 pos_;
    glm::vec3 target_;
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

    void adjustPitch(float offset);

    void adjustYaw(float offset);

  public:

    Camera();

    virtual ~Camera();

    [[nodiscard]] const glm::mat4& matrix();

    [[nodiscard]] float distance() const;

    bool handleMouseButton(int button, int action) override;
    bool handleScroll(float offset) override;
    bool handleMouseMoved(float x, float y) override;
    bool handleKey(int key, int action, int mods) override;

    void setFrontView();
    void setTopView();
    void setSideView();
    void setPerspectiveView();
};

} // namespace recastx::gui

#endif // GUI_CAMERA3D_H