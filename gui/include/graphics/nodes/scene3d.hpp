#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "./scene.hpp"
#include "./camera.hpp"

namespace tomcat::gui {

class Scene3d : public Scene {

  public:

    explicit Scene3d(Client* client);

    ~Scene3d() override;

};

}  // namespace tomcat::gui

#endif // GUI_SCENE3D_H