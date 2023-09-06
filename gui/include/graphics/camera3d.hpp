/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
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

#include "graphics/graph_node.hpp"
#include "input_handler.hpp"
#include "path.hpp"

namespace recastx::gui {

class Camera : public InputHandler {

    glm::vec3 target_;
    glm::vec3 pos0_;
    glm::vec3 front0_;
    glm::vec3 up0_;
    glm::vec3 right0_;
    std::optional<float> distance_ { std::nullopt };

    glm::mat4 rotation_;
    std::optional<glm::mat4> view_ { std::nullopt };

    bool fixed_ = false;

    float mouse_scroll_sensitivity_ = 0.05f;
    float mouse_move_sensitivity_ = 5.f;
    float key_sensitivity_ = 0.2f;

    bool initialized_ = false;
    float prev_x_ = 0.f;
    float prev_y_ = 0.f;

    bool dragging_ = false;

    void adjustPitch(float offset);
    void adjustYaw(float offset);

  public:

    explicit Camera(const glm::vec3& target = glm::vec3 {0.f, 0.f, 0.f});

    virtual ~Camera();

    void render();

    [[nodiscard]] const glm::mat4& matrix();

    [[nodiscard]] float distance();

    [[nodiscard]] bool isFixed() const { return fixed_; }

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