/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SCENE3D_H
#define GUI_SCENE3D_H

#include <memory>

#include "graphics/scene.hpp"
#include "graphics/camera3d.hpp"
#include "control.pb.h"
#include "scene.hpp"

namespace recastx::gui {

class AxiscubeItem;
class AxesItem;
class IconItem;
class PreprocItem;
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
    std::unique_ptr<PreprocItem> preproc_item_;
    std::unique_ptr<ProjectionItem> projection_item_;
    std::unique_ptr<ReconItem> recon_item_;
    std::unique_ptr<StatusbarItem> statusbar_item_;
    std::unique_ptr<LoggingItem> logging_item_;

    std::unique_ptr<AxiscubeItem> axiscube_item_;

    rpc::ServerState_State server_state_;
    rpc::ScanMode_Mode  scan_mode_;
    uint32_t scan_update_interval_;

    void onStateChanged(rpc::ServerState_State state);

    bool updateServerParams();

    bool setScanMode();

public:

    explicit Scene3d(RpcClient* client);

    ~Scene3d() override;

    void onFrameBufferSizeChanged(int width, int height) override;

    void render() override;
};

}  // namespace recastx::gui

#endif // GUI_SCENE3D_H