#include <imgui.h>

#include "graphics/items/logging_item.hpp"
#include "graphics/scene.hpp"
#include "graphics/style.hpp"

namespace tomcat::gui {

LoggingItem::LoggingItem(Scene& scene) :
        GraphicsDataItem(scene),
        log_levels_({"Debug", "Info" ,"Warn", "Error"}){
    scene.addItem(this);

    buf_.emplace_back("[Info] Logging initialized");
}

LoggingItem::~LoggingItem() = default;

void LoggingItem::renderIm() {
    ImGui::Checkbox("Show logging", &visible_);

    if (visible_) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 0.02f));

        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);

        ImGui::Begin("Logging", NULL, ImGuiWindowFlags_NoDecoration);

        // Main window
        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth(100);
        ImGui::Combo("Log levels", &current_level_, log_levels_.data(), log_levels_.size());
        ImGui::PopItemWidth();
        ImGui::SameLine();
        bool clear_text = ImGui::Button("Clear");
        ImGui::SameLine();
        filter_.Draw("Filter", -200.f);

        ImGui::BeginChild("scrolling", ImVec2(0, 0), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        if (clear_text) clear();

        if (filter_.IsActive()) {
            for (auto& line: buf_) {
                auto s = line.data();
                auto e = s + line.size();
                if (filter_.PassFilter(s, e))
                    ImGui::TextUnformatted(s, e);
            }
        } else {
            for (auto& line: buf_) {
                auto s = line.data();
                auto e = s + line.size();
                ImGui::TextUnformatted(s, e);
            }
        }

        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleColor();
    }
}

void LoggingItem::renderGl(const glm::mat4& /*view*/,
                           const glm::mat4& /*projection*/,
                           const RenderParams& /*params*/) {}

void LoggingItem::onWindowSizeChanged(int width, int height) {
    size_ = {
        Style::LOGGING_WIDTH * (float)width, (float)height
    };

    pos_ = {
        (3 * Style::MARGIN + Style::LEFT_PANEL_WIDTH + Style::STATUS_BAR_WIDTH) * (float)width,
        (1.f - Style::BOTTOM_PANEL_HEIGHT - Style::MARGIN) * (float)(height)
    };
}

bool LoggingItem::consume(const ReconDataPacket& packet) {
    return true;
}

void LoggingItem::clear() {
    buf_.clear();
}

} // tomcat::gui
