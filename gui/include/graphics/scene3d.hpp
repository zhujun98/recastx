#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "graphics/scene.hpp"
#include "graphics/camera3d.hpp"
#include "control.pb.h"

namespace recastx::gui {

class AxiscubeItem;
class AxesItem;
class IconItem;
class ProjectionItem;
class ReconItem;
class StatusbarItem;
class LoggingItem;

class Scene3d : public Scene {

    std::unique_ptr<Viewport> viewport_;
    std::unique_ptr<Viewport> viewport_icon_;
    std::unique_ptr<Viewport> viewport_axiscube_;

    std::unique_ptr<AxesItem> axes_item_;
    std::unique_ptr<IconItem> icon_item_;
    std::unique_ptr<ProjectionItem> projection_item_;
    std::unique_ptr<ReconItem> recon_item_;
    std::unique_ptr<StatusbarItem> statusbar_item_;
    std::unique_ptr<LoggingItem> logging_item_;

    std::unique_ptr<AxiscubeItem> axiscube_item_;

    ServerState_State state_;

    void onStateChanged(ServerState_State state);

public:

    explicit Scene3d();

    ~Scene3d() override;

    void init() override;

    void onFrameBufferSizeChanged(int width, int height) override;

    void render() override;
};

}  // namespace recastx::gui

#endif // GUI_SCENE3D_H