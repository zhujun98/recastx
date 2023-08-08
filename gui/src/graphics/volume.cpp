/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/volume.hpp"
#include "graphics/primitives.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

Volume::Volume() : size_({128, 128, 128}) {
    DataType data(size_[0] * size_[1] * size_[2], 0);
    setData(std::move(data), size_);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::cube), primitives::cube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "shaders/recon_volume.vert"
    ;
    auto frag =
#include "shaders/recon_volume.frag"
    ;
    shader_ = std::make_unique<ShaderProgram>(vert, frag);
};

Volume::~Volume() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}

void Volume::setData(DataType&& data, const SizeType& size) {
    data_ = std::move(data);
    size_ = size;

    updateMinMaxVal();

    texture_.setData(data_,
                     static_cast<int>(size_[0]),
                     static_cast<int>(size_[1]),
                     static_cast<int>(size_[2]));
}

void Volume::render(const glm::mat4& view,
                    const glm::mat4& projection,
                    float min_v,
                    float max_v) {
    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("volumeData", 2);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    shader_->setFloat("alpha", 0.2);
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    texture_.bind();
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    texture_.unbind();
}

void Volume::bind() const { texture_.bind(); }
void Volume::unbind() const { texture_.unbind(); }

void Volume::updateMinMaxVal() {
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

const std::array<float, 2>& Volume::minMaxVals() const { return min_max_vals_; }

} // namespace recastx::gui