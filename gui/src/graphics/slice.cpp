/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "graphics/slice.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

Slice::Slice(int slice_id, Plane plane) : id_(slice_id), plane_(plane) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::square), primitives::square, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    auto vert =
#include "shaders/recon_slice.vert"
    ;
    auto frag =
#include "shaders/recon_slice.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    reset();
}

Slice::~Slice() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
};

int Slice::id() const { return id_; }

bool Slice::setData(const std::string& data, uint32_t x, uint32_t y) {
    bool resized = false;
    if (x != x_ || y != y_ ) {
        x_ = x;
        y_ = y;
        data_.resize(x_ * y_);
        resized = true;
    }
    assert(data.size() == x_ * y_ * sizeof(DataType::value_type));
    std::memcpy(data_.data(), data.data(), data.size());
    update_texture_ = true;
    return resized;
}

void Slice::preRender() {
    if (update_texture_) {
        if (data_.empty()) {
            texture_.clear();
        } else {
            texture_.setData(data_, static_cast<int>(x_), static_cast<int>(y_));
        }
        update_texture_ = false;
        updateMinMaxVal();
    }
}

void Slice::render(const glm::mat4& view,
                   const glm::mat4& projection,
                   float min_v,
                   float max_v,
                   bool fallback_to_preview) {
    shader_->use();

    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    shader_->setMat4("orientationMatrix", orientation4() * glm::translate(glm::vec3(0.0, 0.0, 1.0)));
    shader_->setBool("highlighted", hovered_ || highlighted_);
    shader_->setBool("empty", !texture_.isReady());
    shader_->setBool("fallback", fallback_to_preview);

    shader_->setInt("colormap", 0);
    shader_->setInt("sliceData", 1);
    shader_->setInt("volumeData", 2);

    texture_.bind();
    if (!texture_.isReady() && !fallback_to_preview) {
        shader_->setVec4("frameColor", frame_color_);

        glBindVertexArray(vao_);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    } else {
        shader_->setFloat("minValue", min_v);
        shader_->setFloat("maxValue", max_v);

        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    texture_.unbind();
}

void Slice::clear() {
    data_.clear();
    x_ = 0;
    y_ = 0;
    update_texture_ = true;
}

void Slice::setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y) {
    clear();

    float orientation[16] = {x.x,  y.x,  base.x, 0.0f,  // 1
                             x.y,  y.y,  base.y, 0.0f,  // 2
                             x.z,  y.z,  base.z, 0.0f,  // 3
                             0.0f, 0.0f, 0.0f,   1.0f}; // 4
    orient_ = glm::transpose(glm::make_mat4(orientation));
}

void Slice::setOrientation(const Slice::Orient4Type& orient) {
    clear();

    orient_ = orient;
}

void Slice::reset() {
    if (plane_ == Plane::YZ) {
        setOrientation(glm::vec3( 0.0f, -1.0f, -1.0f),
                       glm::vec3( 0.0f,  2.0f,  0.0f),
                       glm::vec3( 0.0f,  0.0f,  2.0f));
    } else if (plane_ == Plane::XZ) {
        setOrientation(glm::vec3(-1.0f,  0.0f, -1.0f),
                       glm::vec3( 2.0f,  0.0f,  0.0f),
                       glm::vec3( 0.0f,  0.0f,  2.0f));
    } else if (plane_ == Plane::XY) {
        setOrientation(glm::vec3(-1.0f, -1.0f,  0.0f),
                       glm::vec3( 2.0f,  0.0f,  0.0f),
                       glm::vec3( 0.0f,  2.0f,  0.0f));
    }
}

Orientation Slice::orientation3() const {
    return {
        orient_[0][0], orient_[0][1], orient_[0][2],
        orient_[1][0], orient_[1][1], orient_[1][2],
        orient_[2][0], orient_[2][1], orient_[2][2]
    };
}

void Slice::updateMinMaxVal() {
    if (data_.empty()) {
        min_max_vals_.reset();
        return;
    }
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

} // namespace recastx::gui
