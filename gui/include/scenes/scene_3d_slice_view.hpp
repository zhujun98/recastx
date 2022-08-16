#ifndef GUI_SCENES_SCENE_3D_SLICE_VIEW_H
#define GUI_SCENES_SCENE_3D_SLICE_VIEW_H

#include <memory>

#include "scene_camera3d.hpp"
#include "scene.hpp"

namespace tomcat::gui {

class Scene3dSliceView : public Scene {

  public:

    Scene3dSliceView();
    ~Scene3dSliceView() override;

    void describe() override;

    void draw(glm::mat4 window_matrix) override;
};

}  // tomcat::gui

#endif // GUI_SCENES_SCENE_3D_SLICE_VIEW_H