#ifndef GUI_GRAPHICS_SCENE_OBJECT3D_H
#define GUI_GRAPHICS_SCENE_OBJECT3D_H

#include <memory>

#include "scene_camera_3d.hpp"
#include "scene_object.hpp"

namespace gui {

class SceneObject3d : public SceneObject {

  public:

    SceneObject3d();
    ~SceneObject3d() override;

    void draw(glm::mat4 window_matrix) override;
};

}  // namespace gui

#endif // GUI_GRAPHICS_SCENE_OBJECT3D_H