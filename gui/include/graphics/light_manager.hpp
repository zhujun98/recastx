/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHT_COMPONENT_H
#define GUI_LIGHT_COMPONENT_H

#include <memory>
#include <vector>

#include "light.hpp"

namespace recastx::gui {

class LightWidget;

class LightManager {

    std::vector<std::shared_ptr<Light>> lights_;
    std::vector<std::unique_ptr<LightWidget>> widgets_;

  public:

    LightManager();

    ~LightManager();

    std::shared_ptr<Light> addLight();

    void renderGUI();

    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& lights() const { return lights_; }
};

} // namespace recastx::gui

#endif // GUI_LIGHT_COMPONENT_H
