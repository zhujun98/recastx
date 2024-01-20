/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
    const auto& l = scene_.layout();

    pos_ = {
            static_cast<float>(3 * l.mw + l.lw + Style::STATUS_BAR_WIDTH * (float)width),
            static_cast<float>(height - l.bh - l.mh)
    };

    size_ = { static_cast<float>(width - l.mw - pos_[0]), static_cast<float>(l.bh) };
}

void LoggingItem::renderIm() {
    if (!visible_) return;

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

    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

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

void LoggingItem::clear() {
    buf_.clear();
}

} // namespace recastx::gui
