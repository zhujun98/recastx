#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "graphics/nodes/scene.hpp"
#include "graphics/nodes/scene_camera3d.hpp"

namespace tomcat::gui {

class Scene3d : public Scene {

  public:

    explicit Scene3d(Client* client);

    ~Scene3d() override;

};

}  // namespace tomcat::gui

#endif // GUI_SCENE3D_H