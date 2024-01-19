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

// SliceTexture

SliceTexture::SliceTexture() : Texture(), x_(1), y_(1) {
    glGenTextures(1, &texture_id_);

    std::vector<DType> data(x_ * y_, 0);
    genTexture(data);
};

SliceTexture::~SliceTexture() {
    glDeleteTextures(1, &texture_id_);
}

SliceTexture::SliceTexture(SliceTexture&& other) noexcept {
    x_ = other.x_;
    y_ = other.y_;
    texture_id_ = other.texture_id_;
    other.texture_id_ = -1;
}

SliceTexture& SliceTexture::operator=(SliceTexture&& other) noexcept {
    x_ = other.x_;
    y_ = other.y_;
    texture_id_ = other.texture_id_;
    other.texture_id_ = -1;
    return *this;
}

void SliceTexture::setData(const std::vector<DType>& data, int x, int y) {
    x_ = x;
    y_ = y;
    assert((int)data.size() == x * y);
    genTexture(data);
    ready_ = true;
}

void SliceTexture::bind() const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void SliceTexture::unbind() const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SliceTexture::genTexture(const std::vector<DType>& data) {
    // In reference to the hack in the 3D fill_texture below, we
    // have found that 1x1 textures are supported in intel
    // integrated graphics.
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, detail::InternalFormat<DType>(), x_, y_, 0,
                 detail::DataFormat<DType>(), detail::DataType<DType>(), data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}

// VolumeTexture

VolumeTexture::VolumeTexture() : Texture(), x_(8), y_(8), z_(8) {
    glGenTextures(1, &texture_id_);

    std::vector<DType> data(x_ * y_ * z_, 0);
    genTexture(data);
};

VolumeTexture::~VolumeTexture() {
    glDeleteTextures(1, &texture_id_);
}

VolumeTexture::VolumeTexture(VolumeTexture&& other) noexcept {
    x_ = other.x_;
    y_ = other.y_;
    z_ = other.z_;
    texture_id_ = other.texture_id_;
    other.texture_id_ = -1;
}

VolumeTexture& VolumeTexture::operator=(VolumeTexture&& other) noexcept {
    x_ = other.x_;
    y_ = other.y_;
    z_ = other.z_;
    texture_id_ = other.texture_id_;
    other.texture_id_ = -1;
    return *this;
}

void VolumeTexture::setData(const std::vector<DType>& data, int x, int y, int z) {
    x_ = x;
    y_ = y;
    z_ = z;
    assert((int)data.size() == x * y * z);
    genTexture(data);
    ready_ = true;
}

void VolumeTexture::bind() const {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, texture_id_);
}

void VolumeTexture::unbind() const {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void VolumeTexture::genTexture(const std::vector<DType>& data) {
    // This is a hack to prevent segfaults on laptops with integrated intel graphics.
    // For some reason, the i965_dri.so module crashes on textures smaller than 8x8x8 pixels.
    if (x_ < 8 || y_ < 8 || z_ < 8) {
        throw std::runtime_error("Showing a slice smaller than 8x8 pixels is not supported "
                                 "due to bugs in Intel GPU drivers");
    }

    glBindTexture(GL_TEXTURE_3D, texture_id_);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_3D, 0, detail::InternalFormat<DType>(), x_, y_, z_, 0,
                 detail::DataFormat<DType>(), detail::DataType<DType>(), data.data());
    glGenerateMipmap(GL_TEXTURE_3D);

    glBindTexture(GL_TEXTURE_3D, 0);
}

} // namespace recastx::gui
