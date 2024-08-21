/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHT_OBJECT_H
#define GUI_LIGHT_OBJECT_H

#include <vector>

#include <glm/glm.hpp>

#include "simple_object.hpp"
#include "light.hpp"

namespace recastx::gui {

class LightObject : public SimpleObject {

    std::shared_ptr<Light> light_;

    void setLightModel();

  public:

    explicit LightObject(std::shared_ptr<Light> light);

    ~LightObject() override;

    void render(Renderer* renderer) override;
};

} // namespace recastx::gui

#endif // GUI_LIGHT_OBJECT_H
