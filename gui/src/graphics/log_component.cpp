/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/log_component.hpp"

#include "logger.hpp"

namespace recastx::gui {

LogComponent::LogComponent(RpcClient* client) :
        Component(client), log_levels_({"Debug", "Info" ,"Warn", "Error"}){
    GuiLogger::init(buf_);
    log::info("Logging initialized");
}

LogComponent::~LogComponent() = default;

void LogComponent::clear() {
    buf_.clear();
}

void LogComponent::draw(rpc::ServerState_State) {
    ImGui::AlignTextToFramePadding();
    ImGui::PushItemWidth(100);
    ImGui::Combo("Log levels##LOG_COMP", &current_level_, log_levels_.data(), log_levels_.size());
    ImGui::PopItemWidth();
    ImGui::SameLine();
    filter_.Draw("Filter", -200.f);
    ImGui::SameLine();
    bool clear_text = ImGui::Button("Clear##LOG_COMP");

    ImGui::BeginChild("Scrolling##LOG_COMP", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

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
}

} // namespace recastx::gui
