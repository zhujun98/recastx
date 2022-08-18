#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "graphics/scenes/scene.hpp"
#include "graphics/scenes/scene_camera3d.hpp"

namespace tomcat::gui {

class Scene3d : public Scene {

  public:

    Scene3d();
    ~Scene3d() override;

    void describe() override;

    void render(const glm::mat4& window_matrix) override;
};

}  // namespace tomcat::gui

#endif // GUI_SCENE3D_H