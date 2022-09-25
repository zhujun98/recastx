#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "graphics/scene.hpp"
#include "graphics/camera3d.hpp"

namespace tomcat::gui {

class AxiscubeItem;
class AxesItem;
class ReconItem;
class StatusbarItem;

class Scene3d : public Scene {

    std::unique_ptr<Viewport> viewport_;
    std::unique_ptr<Viewport> viewport_axiscube_;

    std::unique_ptr<AxesItem> axes_item_;
    std::unique_ptr<ReconItem> recon_item_;
    std::unique_ptr<StatusbarItem> statusbar_item_;

    std::unique_ptr<AxiscubeItem> axiscube_item_;

public:

    explicit Scene3d(Client* client);

    ~Scene3d() override;

    void init() override;

    void onFrameBufferSizeChanged(int width, int height) override;

    void render() override;
};

}  // namespace tomcat::gui

#endif // GUI_SCENE3D_H