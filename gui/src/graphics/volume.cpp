/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "graphics/iso_surface.hpp"
#include "graphics/light.hpp"
#include "graphics/material.hpp"
#include "graphics/volume.hpp"
#include "graphics/volume_slicer.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

Volume::Volume()
        : render_policy_(RenderPolicy::VOLUME),
          slicer_(new VolumeSlicer(128)),
          iso_surface_(new IsoSurface) {
    auto vert =
#include "shaders/recon_vol.vert"
    ;
    auto frag =
#include "shaders/recon_vol.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("volumeData", 2);

    auto shadow_vert =
#include "shaders/recon_vol_has_shadow.vert"
    ;
    auto shadow_frag =
#include "shaders/recon_vol_has_shadow.frag"
    ;
    shadow_shader_ = std::make_unique<ShaderProgram>(shadow_vert, shadow_frag);

    shadow_shader_->use();
    shadow_shader_->setInt("volumeData", 2);
    shadow_shader_->setInt("shadowTexture", 3);

    auto screen_vert =
#include "shaders/screen.vert"
    ;
    auto screen_frag =
#include "shaders/screen.frag"
    ;
    screen_shader_ = std::make_unique<ShaderProgram>(screen_vert, screen_frag);

    screen_shader_->use();
    screen_shader_->setInt("screenTexture", 3);

    auto iso_vert =
#include "shaders/recon_vol_iso.vert"
    ;
    auto iso_frag =
#include "shaders/recon_vol_iso.frag"
    ;
    iso_shader_ = std::make_unique<ShaderProgram>(iso_vert, iso_frag);

    iso_shader_->use();
    iso_shader_->setInt("colormap", 0);
    iso_shader_->setInt("volumeData", 2);

    setRenderQuality(RenderQuality::MEDIUM);
};

bool Volume::init(uint32_t x, uint32_t y, uint32_t z) {
    if (x != b_x_ || y != b_y_ || z != b_z_) {
        buffer_.resize(x * y * z, DataType::value_type(0));
        b_x_ = x;
        b_y_ = y;
        b_z_ = z;
        return true;
    }
    return false;
}

void Volume::setData(const std::string& data, uint32_t x, uint32_t y, uint32_t z) {
    if (x != x_ || y != y_ || z != z_) {
        x_ = x;
        y_ = y;
        z_ = z;
        data_.resize(x_ * y_ * z_);
    }
    assert(data.size() == x_ * y_ * z_ * sizeof(DataType::value_type));
    std::memcpy(data_.data(), data.data(), data.size());
    update_texture_ = true;
    iso_surface_->reset();
}

bool Volume::setShard(const std::string& shard, uint32_t pos) {
    if (buffer_.empty()) return false;

    using value_type = DataType::value_type;
    assert(shard.size() == b_x_ * b_y_ * sizeof(value_type));
    assert(pos * sizeof(value_type) + shard.size() <= buffer_.size() * sizeof(value_type));
    std::memcpy(buffer_.data() + pos, shard.data(), shard.size());
    bool ok =  pos * sizeof(value_type) + shard.size() == buffer_.size() * sizeof(value_type);
    if (ok) {
        buffer_.swap(data_);
        if (x_ != b_x_ || y_ != b_y_ || z_ != b_z_) {
            x_ = b_x_;
            y_ = b_y_;
            z_ = b_z_;
            buffer_.resize(x_ * y_ * z_);
        }
        update_texture_ = true;
        iso_surface_->reset();
    }
    return ok;
}

void Volume::preRender() {
    if (update_texture_) {
        if (data_.empty()) {
            texture_.clear();
        } else {
            texture_.setData(data_, static_cast<int>(x_), static_cast<int>(y_), static_cast<int>(z_));
        }
        update_texture_ = false;
        updateMinMaxVal();
    }
}

void Volume::render(const glm::mat4& view,
                    const glm::mat4& projection,
                    float min_v,
                    float max_v,
                    const glm::vec3& view_dir,
                    const glm::vec3& view_pos,
                    const Light& light,
                    const Material& material,
                    const std::shared_ptr<Viewport>& vp) {
    if (!texture_.isReady()) return;

    if (render_policy_ == RenderPolicy::VOLUME) {
        shader_->use();
        shader_->setFloat("alpha", material.alpha);
        shader_->setFloat("minValue", min_v);
        shader_->setFloat("maxValue", max_v);

        shader_->setVec3("viewPos", view_pos);

        const glm::mat4 light_projection = glm::perspective(45.f, 1.f, 1.f, 200.f);
        const glm::mat4 bias = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.5, 0.5, 0.5)),
                                          glm::vec3(0.5, 0.5, 0.5));

        if (global_illumination_) {
            glm::mat4 light_view = glm::lookAt(light.pos, glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f));
            glm::vec3 light_vec = glm::normalize(light.pos);

            bool is_view_inverted = glm::dot(view_dir, light_vec) < 0;
            const glm::vec3 &half_vec = glm::normalize((is_view_inverted ? -view_dir : view_dir) + light_vec);
            slicer_->update(half_vec, is_view_inverted);

            shadow_shader_->use();
            shadow_shader_->setMat4("mvp", projection * view);
            shadow_shader_->setMat4("mvpShadow", bias * light_projection * light_view);
            shadow_shader_->setVec3("lightColor", light.color);
            shadow_shader_->setFloat("threshold", global_illumination_threshold_);
            shadow_shader_->setFloat("minValue", min_v);
            shadow_shader_->setFloat("maxValue", max_v);

            shader_->use();
            shader_->setMat4("mvp", light_projection * light_view);

            glEnable(GL_BLEND);

            texture_.bind();
            slicer_->drawOnBuffer(shadow_shader_.get(), shader_.get(), is_view_inverted);
            texture_.unbind();

            vp->use();
            screen_shader_->use();
            slicer_->drawOnScreen();

            glDisable(GL_BLEND);
        } else {
            shader_->use();
            shader_->setMat4("mvp", projection * view);

            slicer_->update(view_dir, false);

            texture_.bind();
            slicer_->draw();
            texture_.unbind();
        }
    } else if (render_policy_ == RenderPolicy::SURFACE) {
        iso_surface_->polygonize(data_, x_, y_, z_);

        iso_shader_->use();

        iso_shader_->setMat4("mvp", projection * view);

        iso_shader_->setVec3("viewPos", view_pos);
        iso_shader_->setBool("light.isEnabled", light.is_enabled);
        iso_shader_->setVec3("light.pos", light.pos);
        iso_shader_->setVec3("light.ambient", light.ambient);
        iso_shader_->setVec3("light.diffuse", light.diffuse);
        iso_shader_->setVec3("light.specular", light.specular);

        iso_shader_->setVec3("material.ambient", material.ambient);
        iso_shader_->setVec3("material.diffuse", material.diffuse);
        iso_shader_->setVec3("material.specular", material.specular);
        iso_shader_->setFloat("material.alpha", material.alpha);
        iso_shader_->setFloat("material.shininess", material.shininess);

        texture_.bind();
        iso_surface_->draw();
        texture_.unbind();
    }

}

void Volume::clear() {
    data_.clear();
    x_ = 0;
    y_ = 0;
    z_ = 0;
    update_texture_ = true;
}

void Volume::clearBuffer() {
    buffer_.clear();
    b_x_ = 0;
    b_y_ = 0;
    b_z_ = 0;
}

void Volume::setRenderQuality(RenderQuality level) {
    render_quality_ = level;
    if (level == RenderQuality::VERY_LOW) {
        slicer_->resize(128);
        iso_surface_->setVoxelSize(16, 16, 16);
    } else if (level == RenderQuality::LOW) {
        slicer_->resize(256);
        iso_surface_->setVoxelSize(8, 8, 8);
    } else if (level == RenderQuality::MEDIUM) {
        slicer_->resize(512);
        iso_surface_->setVoxelSize(4, 4, 4);
    } else if (level == RenderQuality::HIGH) {
        slicer_->resize(1024);
        iso_surface_->setVoxelSize(2, 2, 2);
    } else if (level == RenderQuality::VERY_HIGH) {
        slicer_->resize(2048);
        iso_surface_->setVoxelSize(1, 1, 1);
    } else {
        throw;
    }
}

void Volume::setRenderPolicy(RenderPolicy policy) {
    render_policy_ = policy;
}

void Volume::setFront(float front) { slicer_->setFront(front); }

void Volume::setIsoValue(float value) { iso_surface_->setValue(value); }

void Volume::updateMinMaxVal() {
    if (data_.empty()) {
        min_max_vals_.reset();
        return;
    }
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

} // namespace recastx::gui