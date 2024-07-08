/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Algorithms for drag-and-drop slices in 3D space are originally from https://github.com/cicwi/RECAST3D.git.
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/

#include "graphics/widgets/render3d_widget.hpp"
#include "graphics/style.hpp"
#include "graphics/volume.hpp"

namespace recastx::gui {

Render3DWidget::Render3DWidget(Volume* volume) : volume_(volume) {
}

void Render3DWidget::render() {
    ImGui::TextColored(Style::CTRL_SECTION_TITLE_COLOR, "RENDERING");

    static int render_quality = static_cast<int>(volume_->renderQuality());
    if (ImGui::SliderInt("Quality", &render_quality, 1, 5)) {
        volume_->setRenderQuality(RenderQuality(render_quality));
    }

    bool cd = false;
    static int render_policy = static_cast<int>(volume_->renderPolicy());
    cd |= ImGui::RadioButton("Volume##RENDER_COMP", &render_policy,
                             static_cast<int>(RenderPolicy::VOLUME));
    ImGui::SameLine();
    cd |= ImGui::RadioButton("ISO Surface##RENDER_COMP", &render_policy,
                             static_cast<int>(RenderPolicy::SURFACE));
    if (cd) volume_->setRenderPolicy(RenderPolicy(render_policy));

    if (render_policy == static_cast<int>(RenderPolicy::VOLUME)) {
        static bool volume_shadow_enabled = volume_->volumeShadowEnabled();

        ImGui::BeginDisabled(volume_shadow_enabled);
        if (ImGui::DragFloat("Front##RECON_VOL", &volume_front_,
                             volume_front_step_, 0.f, 1.f, "%.3f",
                             ImGuiSliderFlags_AlwaysClamp)) {
            volume_->setFront(volume_front_);
        }
        ImGui::EndDisabled();

        static float threshold = volume_->threshold();
        if (ImGui::DragFloat("Threshold##RENDER_COMP", &threshold,
                             volume_front_step_, 0.f, 1.f, "%.3f",
                             ImGuiSliderFlags_AlwaysClamp)) {
            volume_->setThreshold(threshold);
        }

        if (ImGui::Checkbox("Volume Shadow##RENDER_COMP", &volume_shadow_enabled)) {
            volume_->setVolumeShadowEnabled(volume_shadow_enabled);
            if (volume_shadow_enabled) {
                volume_front_ = 0.f;
                volume_->setFront(0.f);
            }
        }

    } else {
        if (ImGui::InputFloat("ISO Value##RENDER_COMP", &iso_value)) {
            volume_->setIsoValue(iso_value);
        }
    }
}

} // namespace recastx::gui
