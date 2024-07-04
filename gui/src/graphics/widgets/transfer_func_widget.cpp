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
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "graphics/widgets/transfer_func_widget.hpp"
#include "logger.hpp"

namespace recastx::gui {

TransferFuncWidget::TransferFuncWidget(Colormap &colormap, Alphamap &alphamap)
    : cm_(colormap), alpha_{{0.f, 0.f}, {1.f, 1.f}}, am_(alphamap) {
}

void TransferFuncWidget::render() {
    renderColormapSelector();
    renderColorbar();
    renderAlphaEditor();
}

void TransferFuncWidget::renderColormapSelector() {
    auto &cmd = Colormap::data();
    if (ImGui::BeginCombo("Colormap##RECON", cmd.GetName(cm_.get()))) {
        for (auto idx: Colormap::options()) {
            const char *name = cmd.GetName(idx);
            if (ImGui::Selectable(name, cm_.get() == idx)) {
                cm_.set(idx);
            }
        }
        ImGui::EndCombo();
    }
}

void TransferFuncWidget::renderColorbar() {
    ImGuiContext &G = *GImGui;
    const ImGuiStyle &style = G.Style;
    ImGuiWindow *window = G.CurrentWindow;
    if (window->SkipItems) return;

    auto cmap = cm_.get();
    const ImU32 *keys = GImPlot->ColormapData.GetKeys(cmap);
    const int count = GImPlot->ColormapData.GetKeyCount(cmap);
    const ImVec2 pos = window->DC.CursorPos;
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

void TransferFuncWidget::renderAlphaEditor() {
    if (ImGui::GetCurrentWindow()->SkipItems) return;
    ImGui::ItemSize(ImVec2(0, 0)); // add a little space

    ImGuiContext &G = *GImGui;
    const ImGuiStyle &style = G.Style;

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    canvas_size[1] = 0.4f * canvas_size[0];
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, FRAME_COLOR_);

    auto m_pos = G.IO.MousePos;
    float m_pos_x = static_cast<float>(m_pos.x - canvas_pos.x) / canvas_size.x;
    float m_pos_y = 1.f - static_cast<float>(m_pos.y - canvas_pos.y) / canvas_size.y;
    bool is_hovered = m_pos_x >= 0.f && m_pos_x <= 1.f && m_pos_y >= 0.f && m_pos_y <= 1.f;

    float point_r = std::max(4.f / canvas_size.x, 0.008f);
    float select_r = 2 * point_r;
    auto selected = alpha_.end();
    bool alpha_changed = false;
    if (is_hovered) {
        auto it = alpha_.upper_bound(m_pos_x);
        auto prev_it = std::prev(it);
        if (it->first - m_pos_x < select_r && std::abs(it->second - m_pos_x) < select_r) {
            selected = it;
        } else if (m_pos_x - prev_it->first < select_r && std::abs(prev_it->second - m_pos_y) < select_r) {
            selected = prev_it;
        }

        // select and press D to delete a point
        if (selected != alpha_.end() && selected != alpha_.begin() && selected != std::prev(alpha_.end())
            && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D))) {
            alpha_.erase(selected);
            selected = alpha_.end();
            alpha_changed = true;
        }

        // ctrl + left mouse click to add a new point
        if (selected == alpha_.end() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (G.IO.KeyCtrl) {
                if (alpha_.size() > MAX_NUM_POINTS_) {
                    log::warn("Maximum number of points ({}) reached in alphamap!", MAX_NUM_POINTS_);
                } else {
                    alpha_[m_pos_x] = m_pos_y;
                    alpha_changed = true;
                }
            }
        }
    }

    // draw

    const ImVec2 scale(canvas_size.x, -canvas_size.y);
    const ImVec2 offset(canvas_pos.x, canvas_pos.y + canvas_size.y);

    std::vector<ImVec2> points;
    for (auto [x, y]: alpha_) {
        const ImVec2 pos = {x * scale.x + offset.x, y * scale.y + offset.y};
        points.push_back(pos);
    }
    // draw points after drawing the line
    draw_list->AddPolyline(points.data(), (int) points.size(), LINE_COLOR_, false, LINE_WIDTH_);
    for (auto &point: points) {
        draw_list->AddCircleFilled(point, point_r * scale.x, POINT_COLOR_);
    }

    if (selected != alpha_.end()) {
        const ImVec2 pos = {selected->first * scale.x + offset.x, selected->second * scale.y + offset.y};
        draw_list->AddCircle(pos, select_r * scale.x, CIRCLE_COLOR_);
    }

    ImGui::ItemSize(canvas_size, style.FramePadding.y);
    ImGui::Text("Alphamap");

    if (alpha_changed) am_.set(alpha_);
}

} // namespace recastx::gui