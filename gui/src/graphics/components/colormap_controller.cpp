#include "graphics/components/colormap_controller.hpp"

namespace tomcat::gui {

ColormapController::ColormapController() : curr_(*Colormap::options().begin()) {
    cm_.updateTexture(curr_);
};

ColormapController::~ColormapController() = default;

void ColormapController::renderIm() {
    auto& cmd = Colormap::data();
    auto prev_map_ = curr_;
    if (ImGui::BeginCombo("Colormap##ReconComponent", cmd.GetName(curr_))) {
        for (auto idx : Colormap::options()) {
            const char* name = cmd.GetName(idx);
            if (ImGui::Selectable(name, curr_ == idx)) curr_ = idx;
        }
        ImGui::EndCombo();
    }

    if (prev_map_ != curr_) cm_.updateTexture(curr_);
}

const Colormap& ColormapController::colormap() const { return cm_; }

} // namespace tomcat::gui