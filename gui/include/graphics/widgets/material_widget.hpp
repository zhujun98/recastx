/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MATERIAL_WIDGET_H
#define GUI_MATERIAL_WIDGET_H

#include <map>
#include <memory>
#include <string>

#include "imgui.h"
#include <glm/glm.hpp>

#include "widget.hpp"

namespace recastx::gui {

class MeshMaterial;

class MeshMaterialWidget : public Widget {

    std::shared_ptr<MeshMaterial> material_;

    glm::vec3 ambient_;
    glm::vec3 diffuse_;
    glm::vec3 specular_;
    bool initialized_ = false;

  public:

    explicit MeshMaterialWidget(std::shared_ptr<MeshMaterial> material);

    ~MeshMaterialWidget() override;

    void draw() override;
};

class TransferFunc;

class TransferFuncWidget : public Widget {

    std::shared_ptr<TransferFunc> material_;

    std::map<float, float> alpha_;
    bool dragging_ {false};
    std::map<float, float>::iterator dragged_point_;
    const size_t max_num_points_;

    const ImColor frame_color_ {180, 180, 180, 255};
    const ImColor point_color_ {180, 180, 90, 255};
    const ImColor line_color_ {180, 180, 120, 255};
    const float   line_width_ = 2.f;
    const ImColor highlight_color_ {255, 0, 0, 255};

    void renderSelector();

    void renderColorbar();

    void renderLevelsControl();

    void renderAlphaEditor();

  public:

    explicit TransferFuncWidget(std::shared_ptr<TransferFunc> material);

    ~TransferFuncWidget() override;

    void draw() override;
};

} // namespace recastx::gui

#endif // GUI_MATERIAL_WIDGET_H