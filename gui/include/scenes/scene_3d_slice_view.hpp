#ifndef GUI_GRAPHICS_SCENE3D_H
#define GUI_GRAPHICS_SCENE3D_H

#include <memory>

#include "scene_camera3d.hpp"
#include "scene.hpp"

namespace gui {

class Scene3dSliceView : public Scene {

  public:

    Scene3dSliceView();
    ~Scene3dSliceView() override;

    void draw(glm::mat4 window_matrix) override;
};

}  // namespace gui

#endif // GUI_GRAPHICS_SCENE3D_H