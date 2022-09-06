#ifndef GUI_SCENE_CAMERA_H
#define GUI_SCENE_CAMERA_H

#include "glm/glm.hpp"

#include "input_handler.hpp"
#include "ticker.hpp"

namespace tomcat::gui {


class SceneCamera : public InputHandler, public Ticker {

  protected:

    bool interaction_disabled_ = false;

  public:

    SceneCamera();
    virtual ~SceneCamera();

    virtual glm::mat4 matrix() = 0;

    virtual void lookAt(glm::vec3 /* center */) = 0;

    virtual void describe() = 0;

    void toggle_interaction() { interaction_disabled_ = ! interaction_disabled_; }
};

} // namespace tomcat::gui

#endif // GUI_SCENE_CAMERA_H
