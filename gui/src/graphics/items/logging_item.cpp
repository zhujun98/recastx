/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/logging_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"

#include "logger.hpp"

namespace recastx::gui {

LoggingItem::LoggingItem(Scene& scene) :
        GraphicsItem(scene),
        log_levels_({"Debug", "Info" ,"Warn", "Error"}){
    scene.addItem(this);

    GuiLogger::init(buf_);
    log::info("Logging initialized");
}

LoggingItem::~LoggingItem() = default;

void LoggingItem::onWindowSizeChanged(int width, int height) {
    size_ = {
            Style::LOGGING_WIDTH * (float)width,
            Style::LOGGING_HEIGHT * (float)height
    };

    pos_ = {
            (3 * Style::MARGIN + Style::LEFT_PANEL_WIDTH + Style::STATUS_BAR_WIDTH) * (float)width,
            (1.f - Style::STATUS_BAR_HEIGHT - Style::MARGIN) * (float)(height)
    };
}

void LoggingItem::renderIm() {
    ImGui::Checkbox("Show logging", &visible_);

    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Logging", NULL, ImGuiWindowFlags_NoDecoration);

        // Main window
        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth(100);
        ImGui::Combo("Log levels", &current_level_, log_levels_.data(), log_levels_.size());
        ImGui::PopItemWidth();
        ImGui::SameLine();
        filter_.Draw("Filter", -200.f);
        ImGui::SameLine();
        bool clear_text = ImGui::Button("Clear");

        ImGui::BeginChild("scrolling", ImVec2(0, 0), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        if (clear_text) clear();

        if (filter_.IsActive()) {
            for (auto& line: buf_) {
                std::string_view msg = std::get<1>(line);
                auto s = msg.data();
                auto e = s + msg.size();
                if (filter_.PassFilter(s, e))
                    print(std::get<0>(line), s, e);
            }
        } else {
            for (auto& line: buf_) {
                std::string_view msg = std::get<1>(line);
                auto s = msg.data();
                auto e = s + msg.size();
                print(std::get<0>(line), s, e);
            }
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::Separator();
}

void LoggingItem::clear() {
    buf_.clear();
}

} // namespace recastx::gui
