/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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

class Texture {

protected:

    bool initialized_ = false;

    GLenum target_;
    GLenum texture_unit_ = GL_TEXTURE0;
    GLuint texture_id_ = 0;

public:

    Texture(GLenum target, GLenum texture_unit);
    virtual ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void bind() const {
        glActiveTexture(texture_unit_);
        glBindTexture(target_, texture_id_);
    }

    void unbind() const {
        glActiveTexture(texture_unit_);
        glBindTexture(target_, 0);
    }

    [[nodiscard]] bool isInitialized() const { return initialized_; }

    void clear() { initialized_ = false; }

    [[nodiscard]] GLuint texture() const { return texture_id_; }
};

class ColormapTexture : public Texture {

    void genTexture(const std::vector<unsigned char>& data);

public:

    ColormapTexture();

    void setData(const std::vector<unsigned char>& data);
};

class AlphamapTexture : public Texture {

    void genTexture(const std::vector<float>& data);

  public:

    AlphamapTexture();

    void setData(const std::vector<float>& data);
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

    void setData(const std::vector<DType>& data, int x, int y);
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

    void setData(const std::vector<DType>& data, int x, int y, int z);
};


namespace details {

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

template <typename T = unsigned char>
class  ImageTexture : public Texture {

    int x_;
    int y_;

    void genTexture(const std::vector<T>& data) {
        glActiveTexture(texture_unit_);
        glBindTexture(target_, texture_id_);

        glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(target_, 0, details::InternalFormat<T>(), x_, y_, 0,
                     details::DataFormat<T>(), details::DataType<T>(), data.data());

        glBindTexture(target_, 0);
    }

  public:

    ImageTexture() : Texture(GL_TEXTURE_2D, GL_TEXTURE3), x_(1), y_(1) {
        std::vector<T> data(x_ * y_, 0);
        genTexture(data);
    };

    void setData(const std::vector<T>& data, int x, int y) {
        x_ = x;
        y_ = y;
        assert((int)data.size() == x * y);
        genTexture(data);
        initialized_ = true;
    }

    [[nodiscard]] int x() const { return x_; }
    [[nodiscard]] int y() const { return y_; }
};

}  // namespace recastx::gui

#endif // GUI_TEXTURES_H