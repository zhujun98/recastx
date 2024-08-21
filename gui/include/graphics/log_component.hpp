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

#include "logger.hpp"
#include "component.hpp"

namespace recastx::gui {

class LogComponent : public Component {

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

    explicit LogComponent(RpcClient* client);

    ~LogComponent();

    void clear();

    void draw(rpc::ServerState_State state) override;
};

} // namespace recastx::gui

#endif //GUI_LOGGINGITEM_HPP
