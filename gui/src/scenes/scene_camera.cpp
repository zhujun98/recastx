#include <algorithm>
#include <iostream>
#include <vector>

#include <imgui.h>

#include "scenes/scene_camera.hpp"

namespace gui {

SceneCamera::SceneCamera() : curr_cm_("hot") {
    glGenTextures(1, &cm_texture_id_);
    setColormap(curr_cm_);
}

SceneCamera::~SceneCamera() = default;

void SceneCamera::setColormap(const std::string& name) {
    constexpr int samples = 100;

    auto& gradient = SceneCamera::gradients().at(name);

    curr_cm_ = name;
    auto interpolate =
        [](double z, std::vector<std::pair<double, double>>& xys) -> double {
        for (int i = 1; i < (int)xys.size(); ++i) {
            if (z > xys[i].first)
                continue;

            auto val =
                xys[i - 1].second +
                ((z - xys[i - 1].first) / (xys[i].first - xys[i - 1].first)) *
                    (xys[i].second - xys[i - 1].second);

            return val;
        }

        return 0.0f;
    };

    unsigned char image[samples * 3];
    for (int j = 0; j < samples; ++j) {
        for (int i = 0; i < 3; ++i) {
            double intensity = (double)j / samples;
            image[j * 3 + i] = (unsigned char)(255 * interpolate(intensity, gradient[i]));
        }
    }

    glBindTexture(GL_TEXTURE_1D, cm_texture_id_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, samples, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SceneCamera::describe() {
    const std::string prev_cm = curr_cm_;
    if (ImGui::BeginCombo("Colormap", curr_cm_.c_str())) {
        for (auto& gradient : SceneCamera::gradients()) {
          bool is_selected = (curr_cm_ == gradient.first);
          if (ImGui::Selectable(gradient.first.c_str(), is_selected))
              curr_cm_ = gradient.first;
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (prev_cm != curr_cm_) setColormap(curr_cm_);
}

} // namespace gui
