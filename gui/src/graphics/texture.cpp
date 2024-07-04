/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/textures.hpp"

namespace recastx::gui {

Texture::Texture(GLenum target, GLenum texture_unit)
    : target_(target), texture_unit_(texture_unit) {
    glGenTextures(1, &texture_id_);
}

Texture::~Texture() {
    glDeleteTextures(1, &texture_id_);
};

Texture::Texture(Texture&& other) noexcept
    : target_(other.target_),
    texture_unit_(other.texture_unit_),
    texture_id_(std::exchange(other.texture_id_, 0)) {
}

Texture& Texture::operator=(Texture&& other) noexcept {
    target_ = other.target_;
    texture_unit_ = std::exchange(other.texture_unit_, GL_TEXTURE0);
    texture_id_ = std::exchange(other.texture_id_, 0);
    return *this;
}

// ColormapTexture

ColormapTexture::ColormapTexture() : Texture(GL_TEXTURE_1D, GL_TEXTURE4) {
    std::vector<unsigned char> data(1, 0);
    genTexture(data);
};

void ColormapTexture::setData(const std::vector<unsigned char>& data) {
    genTexture(data);
    initialized_ = true;
}

void ColormapTexture::genTexture(const std::vector<unsigned char>& data) {
    glActiveTexture(texture_unit_);
    glBindTexture(target_, texture_id_);

    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    // 3 channels: RGB
    glTexImage1D(target_, 0, GL_RGB32F, data.size() / 3, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(target_);

    glBindTexture(target_, 0);
}

// AlphamapTexture

AlphamapTexture::AlphamapTexture() : Texture(GL_TEXTURE_1D, GL_TEXTURE5) {
    std::vector<float> data{0.f, 1.f};
    genTexture(data);
};

void AlphamapTexture::setData(const std::vector<float>& data) {
    genTexture(data);
    initialized_ = true;
}

void AlphamapTexture::genTexture(const std::vector<float>& data) {
    glActiveTexture(texture_unit_);
    glBindTexture(target_, texture_id_);

    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexImage1D(target_, 0, GL_R32F, data.size(), 0, GL_RED, GL_FLOAT, data.data());

    glGenerateMipmap(target_);

    glBindTexture(target_, 0);
}

// SliceTexture

SliceTexture::SliceTexture() : Texture(GL_TEXTURE_2D, GL_TEXTURE1), x_(1), y_(1) {
    std::vector<DType> data(x_ * y_, 0);
    genTexture(data);
};

void SliceTexture::setData(const std::vector<DType>& data, int x, int y) {
    x_ = x;
    y_ = y;
    assert((int)data.size() == x * y);
    genTexture(data);
    initialized_ = true;
}

void SliceTexture::genTexture(const std::vector<DType>& data) {
    // In reference to the hack in the 3D fill_texture below, we
    // have found that 1x1 textures are supported in intel
    // integrated graphics.
    glActiveTexture(texture_unit_);
    glBindTexture(target_, texture_id_);

    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(target_, 0, GL_R32F, x_, y_, 0, GL_RED, GL_FLOAT, data.data());
    glGenerateMipmap(target_);

    glBindTexture(target_, 0);
}

// VolumeTexture

VolumeTexture::VolumeTexture() : Texture(GL_TEXTURE_3D, GL_TEXTURE2), x_(8), y_(8), z_(8) {
    std::vector<DType> data(x_ * y_ * z_, 0);
    genTexture(data);
};

void VolumeTexture::setData(const std::vector<DType>& data, int x, int y, int z) {
    x_ = x;
    y_ = y;
    z_ = z;
    assert((int)data.size() == x * y * z);
    genTexture(data);
    initialized_ = true;
}

void VolumeTexture::genTexture(const std::vector<DType>& data) {
    // This is a hack to prevent segfaults on laptops with integrated intel graphics.
    // For some reason, the i965_dri.so module crashes on textures smaller than 8x8x8 pixels.
    if (x_ < 8 || y_ < 8 || z_ < 8) {
        throw std::runtime_error("Showing a slice smaller than 8x8 pixels is not supported "
                                 "due to bugs in Intel GPU drivers");
    }

    glActiveTexture(texture_unit_);
    glBindTexture(target_, texture_id_);

    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(target_, 0, GL_R32F, x_, y_, z_, 0, GL_RED, GL_FLOAT, data.data());
    glGenerateMipmap(target_);

    glBindTexture(target_, 0);
}

} // namespace recastx::gui
