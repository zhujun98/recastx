/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LOGGINGITEM_HPP
#define GUI_LOGGINGITEM_HPP

#include <vector>

#include "graphics/items/graphics_item.hpp"
#include "logger.hpp"

namespace recastx::gui {

class LoggingItem : public GraphicsItem {

    ImVec2 pos_;
    ImVec2 size_;

    std::vector<const char*> log_levels_;
    int current_level_;

    GuiLogger::BufferType buf_;
    ImGuiTextFilter filter_;

    inline static constexpr std::string_view level_names[] = {
            "[trace]", "[debug]", "[info]", "[warning]", "[error]"
    };

    static void print(spdlog::level::level_enum level, const char* s, const char* e) {
        ImGui::TextUnformatted(level_names[static_cast<int>(level)].data());
        ImGui::SameLine();
        ImGui::TextUnformatted(s, e);
    }

public:

    explicit LoggingItem(Scene& scene);

    ~LoggingItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void clear();
};

} // namespace recastx::gui

#endif //GUI_LOGGINGITEM_HPP
