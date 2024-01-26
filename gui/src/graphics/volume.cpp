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

#include "graphics/light.hpp"
#include "graphics/volume.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/viewport.hpp"

namespace recastx::gui {

VolumeSlicer::VolumeSlicer(size_t num_slices)
        : shadow_(800, 800), num_slices_(num_slices), slices_(12 * num_slices) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    init();
}

VolumeSlicer::~VolumeSlicer() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void VolumeSlicer::resize(size_t num_slices) {
    if (num_slices != num_slices_) {
        num_slices_ = num_slices;
        slices_.resize(12 * num_slices);
        init();
    }
}


void VolumeSlicer::update(const glm::vec3& view_dir, bool inverted) {
    auto [min_dist, max_dist, max_index] = sortVertices(view_dir);
    float cutoff = min_dist + (max_dist - min_dist) * front_;

    glm::vec3 vec_start[12];
    glm::vec3 vec_dir[12];
    float lambda[12];
    float lambda_inc[12];
    float denom;

    float plane_dist = min_dist;
    float plane_dist_inc = (max_dist - min_dist) / float(num_slices_);

    for (int e = 0; e < 12; e++) {
        auto edge = egs_[pts_[max_index][e]];

        vec_start[e] = vts_[edge[0]];
        vec_dir[e] = vts_[edge[1]] - vec_start[e];

        denom = glm::dot(vec_dir[e], view_dir);
        if (denom != 0.0) {
            lambda_inc[e] =  plane_dist_inc / denom;
            lambda[e]     = (plane_dist - glm::dot(vec_start[e], view_dir)) / denom;
        } else {
            lambda[e]     = -1.0f;
            lambda_inc[e] =  0.0f;
        }
    }

    glm::vec3 intersection[6];
    float dl[12];

    int count = 0;
    for (int i = num_slices_ - 1; i >= 0; i--) {
        if (!inverted && i * plane_dist_inc + min_dist < cutoff) continue;
        else if (inverted && max_dist - i * plane_dist_inc < cutoff) continue;

        for (int e = 0; e < 12; e++) dl[e] = lambda[e] + i * lambda_inc[e];

        if  (dl[0] >= 0.0 && dl[0] < 1.0)	{
            intersection[0] = vec_start[0] + dl[0] * vec_dir[0];
        } else if (dl[1] >= 0.0 && dl[1] < 1.0)	{
            intersection[0] = vec_start[1] + dl[1] * vec_dir[1];
        } else if (dl[3] >= 0.0 && dl[3] < 1.0)	{
            intersection[0] = vec_start[3] + dl[3] * vec_dir[3];
        } else {
            continue;
        }

        if (dl[2] >= 0.0 && dl[2] < 1.0){
            intersection[1] = vec_start[2] + dl[2] * vec_dir[2];
        } else {
            intersection[1] = intersection[0];
        }

        if  (dl[4] >= 0.0 && dl[4] < 1.0){
            intersection[2] = vec_start[4] + dl[4] * vec_dir[4];
        } else if (dl[5] >= 0.0 && dl[5] < 1.0){
            intersection[2] = vec_start[5] + dl[5] * vec_dir[5];
        } else {
            intersection[2] = vec_start[7] + dl[7] * vec_dir[7];
        }

        if	(dl[6] >= 0.0 && dl[6] < 1.0){
            intersection[3] = vec_start[6] + dl[6] * vec_dir[6];
        } else {
            intersection[3] = intersection[2];
        }

        if	(dl[8] >= 0.0 && dl[8] < 1.0){
            intersection[4] = vec_start[8] + dl[8] * vec_dir[8];
        } else if (dl[9] >= 0.0 && dl[9] < 1.0){
            intersection[4] = vec_start[9] + dl[9] * vec_dir[9];
        } else {
            intersection[4] = vec_start[11] + dl[11] * vec_dir[11];
        }

        if (dl[10] >= 0.0 && dl[10] < 1.0){
            intersection[5] = vec_start[10] + dl[10] * vec_dir[10];
        } else {
            intersection[5] = intersection[4];
        }

        for (auto id : ids_) slices_[count++] = intersection[id];
    }
}

void VolumeSlicer::draw() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(slices_.size()));
}

void VolumeSlicer::drawOnScreen() {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadow_.eye_texture_);

    glBindVertexArray(shadow_.vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void  VolumeSlicer::drawOnBuffer(ShaderProgram* shadow_shader,
                                 ShaderProgram* volume_shader,
                                 bool is_view_inverted) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    shadow_.bind();

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao_);
    for (size_t i =0; i < slices_.size(); ++i) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadow_.light_texture_);
        shadow_shader->use();
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        if(is_view_inverted) {
            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);
        glBindTexture(GL_TEXTURE_2D, 0);

        volume_shader->use();
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);
    }
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VolumeSlicer::init() {
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(glm::vec3) * slices_.size(),
                 0,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

std::tuple<float, float, int> VolumeSlicer::sortVertices(const glm::vec3& view_dir) {
    float max_dist = 0;
    float min_dist = max_dist;
    int max_index = 0;
    for (size_t i = 0; i < sizeof(vts_) / sizeof(glm::vec3); i++) {
        float dist = glm::dot(view_dir, vts_[i]);

        if(dist > max_dist) {
            max_dist = dist;
            max_index = i;
        }

        if (dist < min_dist) min_dist = dist;
    }

    static constexpr float EPSILON = 0.0001f;
    max_dist += EPSILON;
    min_dist -= EPSILON;

    return {min_dist, max_dist, max_index};
}

void VolumeSlicer::setFront(float front) {
    if (front > front_) {
        for(size_t i = 0; i < slices_.size(); ++i) slices_[i] = glm::vec3{};
    }
    front_ = front;
}


Volume::Volume() : volume_render_quality_(RenderQuality::MEDIUM), slicer_(128) {
    auto vert =
#include "shaders/recon_volume.vert"
    ;
    auto frag =
#include "shaders/recon_volume.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("volumeData", 2);
    shader_->unuse();

    auto shadow_vert =
#include "shaders/shadow.vert"
    ;
    auto shadow_frag =
#include "shaders/shadow.frag"
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
                    const std::shared_ptr<Viewport>& vp) {
    if (!texture_.isReady()) return;

    shader_->use();
    shader_->setFloat("alpha", alpha_);
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    shader_->setVec3("viewPos", view_pos);
    shader_->setBool("light.isEnabled", light.is_enabled);
    shader_->setVec3("light.pos", light.pos);
    shader_->setVec3("light.ambient", light.ambient);
    shader_->setVec3("light.diffuse", light.diffuse);
    shader_->setVec3("light.specular", light.specular);

    const glm::mat4 light_projection = glm::perspective(45.f, 1.f, 1.f, 200.f);
    const glm::mat4 bias = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.5, 0.5, 0.5)),
                                      glm::vec3(0.5, 0.5, 0.5));

    if (global_illumination_) {
        glm::mat4 light_view = glm::lookAt(light.pos, glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f));
        glm::vec3 light_vec = glm::normalize(light.pos);

        bool is_view_inverted = glm::dot(view_dir, light_vec) < 0;
        const glm::vec3 &half_vec = glm::normalize((is_view_inverted ? -view_dir : view_dir) + light_vec);
        slicer_.update(half_vec, is_view_inverted);

        shadow_shader_->use();
        shadow_shader_->setMat4("mvp", projection * view);
        shadow_shader_->setMat4("mvpShadow", bias * light_projection * light_view);
        shadow_shader_->setVec3("lightColor", light.color);
        shadow_shader_->setFloat("threshold", global_illumination_threshold_);
        shadow_shader_->setFloat("minValue", min_v);
        shadow_shader_->setFloat("maxValue", max_v);

        shader_->use();
        shader_->setMat4("view", light_view);
        shader_->setMat4("projection", light_projection);

        glEnable(GL_BLEND);

        texture_.bind();
        slicer_.drawOnBuffer(shadow_shader_.get(), shader_.get(), is_view_inverted);
        texture_.unbind();

        vp->use();
        screen_shader_->use();
        slicer_.drawOnScreen();

        glDisable(GL_BLEND);
    } else {
        shader_->use();
        shader_->setMat4("view", view);
        shader_->setMat4("projection", projection);

        slicer_.update(view_dir, false);

        texture_.bind();
        slicer_.draw();
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
    if (volume_render_quality_ != level) {
        volume_render_quality_ = level;
        if (level == RenderQuality::LOW) {
            slicer_.resize(256);
        } else if (level == RenderQuality::MEDIUM) {
            slicer_.resize(1024);
        } else if (level == RenderQuality::HIGH) {
            slicer_.resize(2048);
        } else {
            throw;
        }
    }
}

void Volume::updateMinMaxVal() {
    if (data_.empty()) {
        min_max_vals_.reset();
        return;
    }
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

} // namespace recastx::gui