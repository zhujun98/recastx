#include "graphics/widgets/colormap_widget.hpp"

namespace tomcat::gui {

ColormapWidget::ColormapWidget() : curr_(*Colormap::options().begin()) {
    cm_.updateTexture(curr_);
};

ColormapWidget::~ColormapWidget() = default;

void ColormapWidget::renderIm() {
    auto& cmd = Colormap::data();
    auto prev_map_ = curr_;
    if (ImGui::BeginCombo("Colormap##Widget", cmd.GetName(curr_))) {
        for (auto idx : Colormap::options()) {
            const char* name = cmd.GetName(idx);
            if (ImGui::Selectable(name, curr_ == idx)) curr_ = idx;
        }
        ImGui::EndCombo();
    }

    if (prev_map_ != curr_) cm_.updateTexture(curr_);
}

const Colormap& ColormapWidget::colormap() const { return cm_; }

} // namespace tomcat::gui