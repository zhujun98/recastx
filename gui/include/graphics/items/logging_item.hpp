#ifndef GUI_LOGGINGITEM_HPP
#define GUI_LOGGINGITEM_HPP

#include <string>
#include <vector>

#include "graphics/items/graphics_item.hpp"

namespace tomcat::gui {

class LoggingItem : public GraphicsItem {

    ImVec2 pos_;
    ImVec2 size_;

    bool visible_ = true;

    std::vector<const char*> log_levels_;
    int current_level_;

    std::vector<std::string> buf_;
    ImGuiTextFilter filter_;

public:

    explicit LoggingItem(Scene& scene);

    ~LoggingItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void clear();
};

} // tomcat::gui

#endif //GUI_LOGGINGITEM_HPP
