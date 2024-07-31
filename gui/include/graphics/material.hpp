/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MATERIAL_H
#define GUI_MATERIAL_H

#include <optional>

#include <glm/glm.hpp>

#include "aesthetics.hpp"

namespace recastx::gui {

using MaterialID = unsigned int;

class Material {

    inline static MaterialID next_id_ = 0;

  protected:

    MaterialID id_;

  public:

    explicit Material() : id_(next_id_++) {
    }

    virtual ~Material() = default;

    [[nodiscard]] MaterialID id() const { return id_; }
};

class MeshMaterial : public Material {

    std::optional<glm::vec3> ambient_;
    Texture2D ambient_tex_;

    std::optional<glm::vec3> diffuse_;
    Texture2D diffuse_tex_;

    std::optional<glm::vec3> specular_;
    Texture2D specular_tex_;

    float shininess_ = 32;

    friend class MeshMaterialWidget;

  public:

    MeshMaterial() = default;

    ~MeshMaterial() override = default;

    void setAmbient(glm::vec3 color) {
        ambient_ = color;
        ambient_tex_ = Texture2D(ambient_.value());
    }

    void setDiffuse(glm::vec3 color) {
        diffuse_ = color;
        diffuse_tex_ = Texture2D(diffuse_.value());
    }

    void setSpecular(glm::vec3 color) {
        specular_ = color;
        specular_tex_ = Texture2D(specular_.value());
    }

    [[nodiscard]] float shininess() const { return shininess_; }
    void setShininess(float value) { shininess_ = value; }

    void bind(unsigned int u1, unsigned int u2, unsigned int u3) {
        ambient_tex_.bind(u1);
        diffuse_tex_.bind(u2);
        specular_tex_.bind(u3);
    }

    void unbind() {
        specular_tex_.unbind();
        diffuse_tex_.unbind();
        ambient_tex_.unbind();
    }
};

class TransferFunc : public Material {

    Colormap cm_;
    Alphamap am_;

    bool alpha_enabled_ { true };

    float min_val_ { 0.f };
    float max_val_ { 0.f };
    bool auto_levels_ { true };
    std::vector<float> min_vals_;
    std::vector<float> max_vals_;

    friend class TransferFuncWidget;

  public:

    TransferFunc() = default;

    ~TransferFunc() override = default;

    void bind(unsigned int u1, unsigned int u2) {
        if (!alpha_enabled_) throw std::runtime_error("Alpha is disabled!");
        cm_.bind(u1);
        am_.bind(u2);
    }

    void unbind() {
        am_.unbind();
        cm_.unbind();
    }

    void bindColor(unsigned int u) { cm_.bind(u); }

    void unbindColor() { cm_.unbind(); }

    template<typename T>
    void registerMinMaxVals(const std::array<T, 2>& value) {
        min_vals_.push_back(static_cast<float>(value[0]));
        max_vals_.push_back(static_cast<float>(value[1]));
    }

    void updateMinMaxVals() {
        if (!auto_levels_) return;
        if (!min_vals_.empty()) min_val_ = *std::min_element(min_vals_.begin(), min_vals_.end());
        if (!max_vals_.empty()) max_val_ = *std::max_element(max_vals_.begin(), max_vals_.end());
    }

    [[nodiscard]] std::array<float, 2> minMaxVals() const { return { min_val_, max_val_}; }

    void setAlphaEnabled(bool state) { alpha_enabled_ = state; }
};

} // namespace recastx::gui

#endif // GUI_MATERIAL_H