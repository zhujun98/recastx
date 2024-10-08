/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_CAMERA3D_H
#define GUI_CAMERA3D_H

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "event.hpp"

namespace recastx::gui {

class Camera {

    glm::vec3 target_;
    glm::vec3 pos_;
    float dist_;
    static constexpr float K_MAX_DIST_ = 10.f;
    static constexpr float K_MIN_DIST_ = 0.5f;

    const glm::vec3 front_;
    const glm::vec3 up_;
    const glm::vec3 right_;

    glm::mat4 rotation_;
    std::optional<glm::mat4> view_ { std::nullopt };

    float mouse_scroll_sensitivity_ = 0.05f;
    float mouse_move_sensitivity_ = 5.f;
    float key_sensitivity_ = 0.2f;

    void rotate(float angle, const glm::vec3& axis);

  public:

    Camera();

    ~Camera();

    [[nodiscard]] const glm::mat4& viewMatrix();

    [[nodiscard]] glm::vec3 viewDir();

    [[nodiscard]] float distance();

    [[nodiscard]] glm::vec3 pos();

    bool mouseDragEvent(const MouseDragEvent& ev);
    bool mouseScrollEvent(const MouseScrollEvent& ev);
    bool keyEvent(const KeyEvent& ev);

    void setFrontView();
    void setTopView();
    void setSideView();
    void setPerspectiveView();
};

} // namespace recastx::gui

#endif // GUI_CAMERA3D_H