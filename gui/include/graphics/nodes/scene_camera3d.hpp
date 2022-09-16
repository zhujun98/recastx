#ifndef GUI_SCENE_CAMERA3D_H
#define GUI_SCENE_CAMERA3D_H

#include <map>
#include <memory>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "scene_camera.hpp"
#include "path.hpp"

namespace tomcat::gui {

class SceneCamera3d;

enum class drag_machine_kind : int {
    none,
    rotator,
};

class CameraDragMachine : public Ticker {
  public:
    explicit CameraDragMachine(SceneCamera3d& camera) : camera_(camera) {}
    virtual ~CameraDragMachine() = default;

    virtual void onDrag(glm::vec2 cur, glm::vec2 delta) = 0;
    virtual drag_machine_kind kind() = 0;

  protected:
    SceneCamera3d& camera_;
};


class Rotator : public CameraDragMachine {

    float x_;
    float y_;
    float cx_;
    float cy_;
    bool instant_ = true;

  public:

    using CameraDragMachine::CameraDragMachine;

    Rotator(SceneCamera3d& camera, float x, float y, bool instant = true);

    void onDrag(glm::vec2 cur, glm::vec2 delta) override;

    void tick(double time_elapsed) override;

    drag_machine_kind kind() override;
};


class SceneCamera3d : public SceneCamera {

    glm::vec3 position_;

    float angle_ = 0.0f;
    float scale_ = 0.5f;

    double prev_x_ = -1.1;
    double prev_y_ = -1.1;
    glm::vec2 delta_;

    bool dragging_ = false;
    bool instant_ = true;

    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 center_;
    glm::mat4 rotation_;

    std::unique_ptr<CameraDragMachine> drag_machine_;

    void setPerspectiveView();

  public:

    SceneCamera3d();
    ~SceneCamera3d() override;

    glm::mat4 matrix() override;

    auto& up() { return up_; }
    auto& right() { return right_; }

    void switch_if_necessary(drag_machine_kind kind);

    bool handleMouseButton(int button, int action) override;
    bool handleScroll(double offset) override;
    bool handleMouseMoved(double x, double y) override;
    bool handleKey(int key, int action, int mods) override;

    void tick(double time_elapsed) override;

    void lookAt(glm::vec3 center) override;

    void rotate(float phi, float psi);
    void describe() override;
};

} // namespace tomcat::gui

#endif // GUI_SCENE_CAMERA3D_H