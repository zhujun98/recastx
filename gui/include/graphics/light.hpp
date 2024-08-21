/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LIGHT_H
#define GUI_LIGHT_H

#include <utility>

#include <glm/glm.hpp>

namespace recastx::gui {

using LightId = unsigned int;

class Light {

    inline static LightId next_id_ = 0;

    LightId id_;

    glm::vec3 rel_pos_; // relative to view position
    glm::vec3 pos_;

    glm::vec3 color_ = {1.f, 1.f, 1.f};
    float ambient_ = 0.f;
    float diffuse_ = 0.f;
    float specular_ = 0.f;

    bool visible_ = false;

    friend class LightWidget;

  public:

    Light() : id_(next_id_++) {}

    ~Light() = default;

    [[nodiscard]] LightId id() const { return id_; }

    void setColor(glm::vec3 rgb) { color_ = rgb; }
    void setAmbient(float value) { ambient_ = value; }
    void setDiffuse(float value) { diffuse_ = value; }
    void setSpecular(float value) { specular_ = value; }

    [[nodiscard]] glm::vec3 color() const { return color_; }
    [[nodiscard]] glm::vec3 ambient() const { return ambient_ * color_; }
    [[nodiscard]] glm::vec3 diffuse() const { return diffuse_ * color_; }
    [[nodiscard]] glm::vec3 specular() const { return specular_ * color_; }

    [[nodiscard]] bool visible() const { return visible_; }

    void setPosition(const glm::vec3& value) { pos_ = value; }
    [[nodiscard]] glm::vec3 position() const { return pos_; }

    void setRelPosition(const glm::vec3& value) { rel_pos_ = value; }
    [[nodiscard]] glm::vec3 relativePosition() const { return rel_pos_; }
};

} // namespace recastx::gui

#endif // GUI_LIGHT_H