/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/widgets/material_widget.hpp"
#include "graphics/material.hpp"
#include "logger.hpp"

namespace recastx::gui {

MeshMaterialWidget::MeshMaterialWidget(std::shared_ptr<MeshMaterial> material)
        : Widget("Material " + std::to_string(material->id()) + " (mesh)"),
          material_(std::move(material)) {
    id_ = "##MAT" + std::to_string(material_->id());
}

MeshMaterialWidget::~MeshMaterialWidget() = default;

void MeshMaterialWidget::draw() {
    ImGui::SliderFloat(("Shininess" + id_).c_str(), &material_->shininess_, 0.f, 100.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

    if (!initialized_) {
        if (material_->ambient_) ambient_ = material_->ambient_.value();
        if (material_->diffuse_) diffuse_ = material_->diffuse_.value();
        if (material_->specular_) specular_ = material_->specular_.value();
        initialized_ = true;
    }

    if (material_->ambient_) {
        if (ImGui::ColorEdit3(("Ambient" + id_).c_str(), &ambient_.x)) {
            material_->setAmbient(ambient_);
        }
    }

    if (material_->diffuse_) {
        if (ImGui::ColorEdit3(("Diffuse" + id_).c_str(), &diffuse_.x)) {
            material_->setDiffuse(diffuse_);
        }
    }

    if (material_->specular_) {
        if (ImGui::ColorEdit3(("Specular" + id_).c_str(), &specular_.x)) {
            material_->setSpecular(specular_);
        }
    }
}


TransferFuncWidget::TransferFuncWidget(std::shared_ptr<TransferFunc> material)
        : Widget("Material " + std::to_string(material->id())),
          material_(std::move(material)),
          alpha_{{0.f, 0.f}, {1.f, 1.f}},
          dragged_point_(alpha_.end()),
          max_num_points_(10) {
    material_->am_.set(alpha_);
    id_ = "##MAT" + std::to_string(material_->id());
}

TransferFuncWidget::~TransferFuncWidget() = default;

void TransferFuncWidget::draw() {
    renderSelector();
    renderColorbar();
    renderLevelsControl();

    if (material_->alpha_enabled_) renderAlphaEditor();
}


void TransferFuncWidget::renderSelector() {
    auto& cmd = Colormap::data();
    auto& cm = material_->cm_;
    if (ImGui::BeginCombo(("Colormap" + id_).c_str(), cmd.GetName(cm.get()))) {
        for (auto idx : Colormap::options()) {
            const char* name = cmd.GetName(idx);
            if (ImGui::Selectable(name, cm.get() == idx)) {
                cm.set(idx);
            }
        }
        ImGui::EndCombo();
    }
}

void TransferFuncWidget::renderColorbar() {
    ImGuiContext& G = *GImGui;
    const ImGuiStyle& style = G.Style;
    ImGuiWindow * window = G.CurrentWindow;
    if (window->SkipItems) return;

    auto cmap = material_->cm_.get();
    const ImU32* keys  = GImPlot->ColormapData.GetKeys(cmap);
    const int count = GImPlot->ColormapData.GetKeyCount(cmap);
    const ImVec2 pos  = window->DC.CursorPos;
    const ImVec2 label_size = ImGui::CalcTextSize("Colormap", NULL, true);
    const float bar_width = ImGui::GetContentRegionAvail().x;
    const float bar_height = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect rect = ImRect(pos.x, pos.y, pos.x + bar_width, pos.y + bar_height);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const float step = rect.GetWidth() / static_cast<float>(count - 1);
    ImRect block(rect.Min.x, rect.Min.y, rect.Min.x + step, rect.Max.y);
    for (int i = 0; i < count - 1; ++i) {
        ImU32 col1 = keys[i];
        ImU32 col2 = keys[i + 1];
        draw_list->AddRectFilledMultiColor(block.Min, block.Max, col1, col2, col2, col1);
        block.TranslateX(step);
    }

    ImGui::ItemSize({bar_width, bar_height}, style.FramePadding.y);
}

void TransferFuncWidget::renderLevelsControl() {
    ImGui::Checkbox(("Auto levels" + id_).c_str(), &material_->auto_levels_);

    ImGui::BeginDisabled(material_->auto_levels_);
    float step_size = (material_->max_val_ - material_->min_val_) / 200.f;
    if (step_size < 0.01f) step_size = 0.01f; // avoid a tiny step size
    ImGui::DragFloatRange2(("Min / Max" + id_).c_str(), &material_->min_val_, &material_->max_val_, step_size,
                           std::numeric_limits<float>::lowest(), // min() does not work
                           std::numeric_limits<float>::max());
    ImGui::EndDisabled();
}

void TransferFuncWidget::renderAlphaEditor() {
    if (ImGui::GetCurrentWindow()->SkipItems) return;

    ImGui::Text("Alphamap");

    ImGuiContext& G = *GImGui;
    const ImGuiStyle& style = G.Style;

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size[1] = 0.4f * canvas_size[0];
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(canvas_pos, {canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y}, frame_color_);

    float radius_p = std::max(4.f / canvas_size.x, 0.008f);
    float radius_s = 2 * radius_p;
    bool alpha_changed = false;

    auto hovered = alpha_.end();

    auto m_pos = G.IO.MousePos;
    float m_pos_x = static_cast<float>(m_pos.x - canvas_pos.x) / canvas_size.x;
    float m_pos_y = 1.f - static_cast<float>(m_pos.y - canvas_pos.y) / canvas_size.y;
    if (m_pos_x >= 0.f && m_pos_x <= 1.f && m_pos_y >= 0.f && m_pos_y <= 1.f) {
        auto it = alpha_.upper_bound(m_pos_x);
        auto prev_it = std::prev(it);
        if (it->first - m_pos_x < radius_s && std::abs(it->second - m_pos_x) < radius_s) {
            hovered = it;
        } else if (m_pos_x - prev_it->first < radius_s && std::abs(prev_it->second - m_pos_y) < radius_s) {
            hovered = prev_it;
        }

        bool mouse_down = ImGui::IsMouseDown(0);
        // The first point is not allowed to be dragged since alpha must be 0 if the density is 0.
        if (mouse_down && !dragging_ && hovered != alpha_.end() && hovered != alpha_.begin()) {
            dragging_ = true;
            dragged_point_ = hovered;
        } else if (dragging_ && !mouse_down) {
            dragging_ = false;
            dragged_point_ = alpha_.end();
        }

        if (dragging_ && ImGui::IsMouseDragging(0)) {
            dragged_point_->second = m_pos_y;
            alpha_changed = true;
        }

        // select and press x to delete a point
        // cannot delete the first and last points
        ImGui::SetNextFrameWantCaptureKeyboard(true);
        if (hovered != alpha_.end() && hovered != alpha_.begin() && hovered != std::prev(alpha_.end())
            && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X))) {
            alpha_.erase(hovered);
            hovered = alpha_.end();
            alpha_changed = true;
        }

        // ctrl + left mouse click to add a new point
        if (hovered == alpha_.end() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (G.IO.KeyCtrl) {
                if (alpha_.size() > max_num_points_) {
                    log::warn("Maximum number of points reached!");
                } else {
                    alpha_[m_pos_x] = m_pos_y;
                    alpha_changed = true;
                }
            }
        }
    }

    ImGui::ItemSize({canvas_size.x, canvas_size.y + style.FramePadding.y});

    const ImVec2 scale(canvas_size.x, -canvas_size.y);
    const ImVec2 offset(canvas_pos.x, canvas_pos.y + canvas_size.y);

    std::vector<ImVec2> points;
    for (auto [x, y] : alpha_) {
        const ImVec2 pos = { x * scale.x + offset.x, y * scale.y + offset.y };
        points.push_back(pos);
    }

    draw_list->AddPolyline(points.data(), (int)points.size(), line_color_, false, line_width_);
    for (auto& point : points) {
        draw_list->AddCircleFilled(point, radius_p * scale.x, point_color_);
    }

    auto highlighted = dragging_? dragged_point_ : hovered;
    if (highlighted != alpha_.end()) {
        const ImVec2 pos = { highlighted->first * scale.x + offset.x, highlighted->second * scale.y + offset.y };
        draw_list->AddCircle(pos, radius_s * scale.x, highlight_color_);
    }

    if (alpha_changed) material_->am_.set(alpha_);
}

} // namespace recastx::gui