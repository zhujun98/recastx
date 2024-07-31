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
    const size_t max_num_points_;

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