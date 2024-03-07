/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUME_H
#define GUI_VOLUME_H

#include <array>
#include <memory>
#include <optional>

#include "style.hpp"
#include "textures.hpp"
#include "utils.hpp"

// FIXME
#include "volume_slicer.hpp"
#include "iso_surface.hpp"

namespace recastx::gui {

class IsoSurface;
struct Light;
struct Material;
class ShaderProgram;
class Viewport;
class VolumeSlicer;

class Volume {

  public:

    using DataType = std::vector<float>;

  private:

    DataType data_;
    uint32_t x_;
    uint32_t y_;
    uint32_t z_;
    bool update_texture_ = false;
    VolumeTexture texture_;

    DataType buffer_;
    uint32_t b_x_;
    uint32_t b_y_;
    uint32_t b_z_;

    std::optional<std::array<float, 2>> min_max_vals_;

    RenderPolicy render_policy_;
    RenderQuality render_quality_;

    std::unique_ptr<VolumeSlicer> slicer_;
    float threshold_ = 0.1f;
    bool volume_shadow_enabled_ = false;

    std::unique_ptr<ShaderProgram> shader_;
    std::unique_ptr<ShaderProgram> vslice_shader_;
    std::unique_ptr<ShaderProgram> vlight_shader_;
    std::unique_ptr<ShaderProgram> screen_shader_;

    std::unique_ptr<IsoSurface> iso_surface_;
    std::unique_ptr<ShaderProgram> iso_shader_;

    void updateMinMaxVal();

public:

    Volume();

    bool init(uint32_t x, uint32_t y, uint32_t z);

    void setData(const std::string& data, uint32_t x, uint32_t y, uint32_t z);

    bool setShard(const std::string& shard, uint32_t pos);

    void preRender();

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v,
                float alpha_scale,
                const glm::vec3& view_dir,
                const glm::vec3& view_pos,
                const Light& light,
                const Material& material,
                const std::shared_ptr<Viewport>& vp);

    [[nodiscard]] bool hasTexture() const { return texture_.isReady(); }

    void clear();

    void clearBuffer();

    void bind() const { texture_.bind(); }

    void unbind() const { texture_.unbind(); }

    [[nodiscard]] const std::optional<std::array<float, 2>>& minMaxVals() const { return min_max_vals_; }

    [[nodiscard]] RenderQuality renderQuality() const { return render_quality_; }

    void setRenderQuality(RenderQuality level);

    [[nodiscard]] RenderPolicy renderPolicy() const { return render_policy_; }

    void setRenderPolicy(RenderPolicy policy);

    void setFront(float front);

    [[nodiscard]] bool volumeShadowEnabled() const { return volume_shadow_enabled_; }

    void setVolumeShadowEnabled(bool state) { volume_shadow_enabled_ = state; }

    [[nodiscard]] float threshold() const { return threshold_; }

    void setThreshold(float value) { threshold_ = value; }

    void setIsoValue(float value);
};

} // namespace recastx::gui

#endif // GUI_VOLUME_H
