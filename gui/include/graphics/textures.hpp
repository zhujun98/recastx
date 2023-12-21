/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_TEXTURES_H
#define GUI_TEXTURES_H

#include <algorithm>
#include <exception>
#include <iostream>
#include <vector>
#include <cstddef>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

namespace recastx::gui {

namespace detail {

template<typename T>
inline GLint InternalFormat();

template <>
inline GLint InternalFormat<float>() { return GL_R32F; }

template <>
inline GLint InternalFormat<unsigned short>() { return GL_R16UI; }

template <>
inline GLint InternalFormat<unsigned char>() { return GL_RGB32F; }

template<typename T>
inline GLenum DataFormat();

template <>
inline GLenum DataFormat<float>() { return GL_RED; }

template <>
inline GLenum DataFormat<unsigned short>() { return GL_RED_INTEGER; }

template <>
inline GLenum DataFormat<unsigned char>() { return GL_RGB; }

template<typename T>
inline GLenum DataType();

template <>
inline GLenum DataType<float>() { return GL_FLOAT; }

template <>
inline GLenum DataType<unsigned short>() { return GL_UNSIGNED_SHORT; }

template <>
inline GLenum DataType<unsigned char>() { return GL_UNSIGNED_BYTE; }

} // namespace detail

class Texture {

protected:

    bool ready_ = false;

    GLuint texture_id_ = 0;

public:

    Texture() = default;
    virtual ~Texture() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    [[nodiscard]] bool isReady() const { return ready_; }

    void clear() { ready_ = false; }

    GLuint texture() const { return texture_id_; }
};

template <typename T = unsigned char>
class ColormapTexture : public Texture {

    int x_;

    void genTexture(const std::vector<T>& data) {
        glBindTexture(GL_TEXTURE_1D, texture_id_);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        glTexImage1D(GL_TEXTURE_1D, 0, detail::InternalFormat<T>(), x_, 0,
                     detail::DataFormat<T>(), detail::DataType<T>(), data.data());
        glGenerateMipmap(GL_TEXTURE_1D);

        glBindTexture(GL_TEXTURE_1D, 0);
    }

public:

    ColormapTexture() : Texture(), x_(1) {
        glGenTextures(1, &texture_id_);

        std::vector<T> data(x_, 0);
        genTexture(data);
    };

    ~ColormapTexture() override {
        glDeleteTextures(1, &texture_id_);
    }

    ColormapTexture(const ColormapTexture&) = delete;
    ColormapTexture& operator=(const ColormapTexture&) = delete;

    ColormapTexture(ColormapTexture&& other) noexcept {
        x_ = other.x_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    ColormapTexture& operator=(ColormapTexture&& other) noexcept {
        x_ = other.x_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
        return *this;
    }

    void setData(const std::vector<T>& data, int x) {
        x_ = x;
        assert((int)data.size() == 3 * x); // 3 channels: RGB
        genTexture(data);
        ready_ = true;
    }

    void bind() const override {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, 0);
    }
};

class SliceTexture : public Texture {

  public:

    using DType = float;

  private:

    int x_;
    int y_;

    void genTexture(const std::vector<DType>& data);

public:

    SliceTexture();

    ~SliceTexture() override;

    SliceTexture(const SliceTexture&) = delete;
    SliceTexture& operator=(const SliceTexture&) = delete;

    SliceTexture(SliceTexture&& other) noexcept;
    SliceTexture& operator=(SliceTexture&& other) noexcept;

    void setData(const std::vector<DType>& data, int x, int y);

    void bind() const override;

    void unbind() const override;
};

class VolumeTexture  : public Texture {

  public:

    using DType = float;

  private:

    int x_;
    int y_;
    int z_;

    void genTexture(const std::vector<DType>& data);

public:

    VolumeTexture();

    ~VolumeTexture() override;

    VolumeTexture(const VolumeTexture&) = delete;
    VolumeTexture& operator=(const VolumeTexture&) = delete;

    VolumeTexture(VolumeTexture&& other) noexcept;
    VolumeTexture& operator=(VolumeTexture&& other) noexcept;

    void setData(const std::vector<DType>& data, int x, int y, int z);

    void bind() const override;

    void unbind() const override;
};

template <typename T = unsigned char>
class  ImageTexture : public Texture {

    int x_;
    int y_;

    void genTexture(const std::vector<T>& data) {
        glBindTexture(GL_TEXTURE_2D, texture_id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, detail::InternalFormat<T>(), x_, y_, 0,
                     detail::DataFormat<T>(), detail::DataType<T>(), data.data());

        glBindTexture(GL_TEXTURE_2D, 0);
    }

  public:

    ImageTexture() : Texture(), x_(1), y_(1) {
        glGenTextures(1, &texture_id_);

        std::vector<T> data(x_ * y_, 0);
        genTexture(data);
    };

    ~ImageTexture() override {
        glDeleteTextures(1, &texture_id_);
    }

    ImageTexture(const ImageTexture&) = delete;
    ImageTexture& operator=(const ImageTexture&) = delete;

    ImageTexture(ImageTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    ImageTexture& operator=(ImageTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
        return *this;
    }

    void setData(const std::vector<T>& data, int x, int y) {
        x_ = x;
        y_ = y;
        assert((int)data.size() == x * y);
        genTexture(data);
        ready_ = true;
    }

    [[nodiscard]] int x() const { return x_; }
    [[nodiscard]] int y() const { return y_; }

    void bind() const override {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

}  // namespace recastx::gui

#endif // GUI_TEXTURES_H