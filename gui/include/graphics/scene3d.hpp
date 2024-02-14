/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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

class AxisItem;
class AxisCubeItem;
class IconItem;
class GeometryItem;
class LoggingItem;
class PreprocItem;
class ProjectionItem;
class ReconItem;
class StatusbarItem;

class Scene3d : public Scene {

    std::unique_ptr<AxisItem> axis_item_;
    std::unique_ptr<AxisCubeItem> axiscube_item_;
    std::unique_ptr<IconItem> icon_item_;
    std::unique_ptr<GeometryItem> geometry_item_;
    std::unique_ptr<LoggingItem> logging_item_;
    std::unique_ptr<PreprocItem> preproc_item_;
    std::unique_ptr<ProjectionItem> projection_item_;
    std::unique_ptr<ReconItem> recon_item_;
    std::unique_ptr<StatusbarItem> statusbar_item_;

    void renderMenubarRight();
    void renderMainControl();
    void renderScanModeControl();
    void renderCameraControl();

public:

    explicit Scene3d(RpcClient* client);

    ~Scene3d() override;

    void render() override;

    bool handleKey(int key, int action, int mods) override;
};

}  // namespace recastx::gui

#endif // GUI_SCENE3D_H