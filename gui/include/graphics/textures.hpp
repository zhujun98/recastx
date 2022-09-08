#ifndef GUI_TEXTURES_H
#define GUI_TEXTURES_H

#include <algorithm>
#include <exception>
#include <iostream>
#include <vector>
#include <cstddef>

#include <GL/gl3w.h>
#include <glm/glm.hpp>

namespace tomcat::gui {

namespace detail {

template<typename T>
inline GLint InternalFormat();

template <>
inline GLint InternalFormat<float>() { return GL_R32F; }

template <>
inline GLint InternalFormat<unsigned char>() { return GL_RGB32F; }

template<typename T>
inline GLenum DataFormat();

template <>
inline GLenum DataFormat<float>() { return GL_RED; }

template <>
inline GLenum DataFormat<unsigned char>() { return GL_RGB; }

template<typename T>
inline GLenum DataType();

template <>
inline GLenum DataType<float>() { return GL_FLOAT; }

template <>
inline GLenum DataType<unsigned char>() { return GL_UNSIGNED_BYTE; }

} // namespace detail

class Texture {

protected:

    GLuint texture_id_ = -1;

public:

    Texture() = default;
    virtual ~Texture() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
};

template <typename T = unsigned char>
class ColormapTexture : public Texture {

    int x_ = 0;

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

    ColormapTexture() : Texture() {
        glGenTextures(1, &texture_id_);
    };

    ~ColormapTexture() override {
        if (texture_id_ >= 0) glDeleteTextures(1, &texture_id_);
    }

    ColormapTexture(ColormapTexture&& other) noexcept {
        x_ = other.x_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    ColormapTexture& operator=(ColormapTexture&& other) noexcept {
        x_ = other.x_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    void setData(const std::vector<T>& data, int x) {
        x_ = x;
        assert((int)data.size() == x);
        genTexture(data);
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

template <typename T = unsigned char>
class SliceTexture : public Texture {

    int x_ = 0;
    int y_ = 0;

    void genTexture(const std::vector<T>& data) {
        // In reference to the hack in the 3D fill_texture below, we
        // have found that 1x1 textures are supported in intel
        // integrated graphics.
        glBindTexture(GL_TEXTURE_2D, texture_id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, detail::InternalFormat<T>(), x_, y_, 0,
                     detail::DataFormat<T>(), detail::DataType<T>(), data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

public:

    SliceTexture() : Texture() {
        glGenTextures(1, &texture_id_);
    };

    ~SliceTexture() override {
        if (texture_id_ >= 0) glDeleteTextures(1, &texture_id_);
    }

    SliceTexture(SliceTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    SliceTexture& operator=(SliceTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    void setData(const std::vector<T>& data, int x, int y) {
        x_ = x;
        y_ = y;
        assert((int)data.size() == x * y);
        genTexture(data);
    }

    void bind() const override {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

template <typename T = unsigned char>
class VolumeTexture  : public Texture {

    int x_ = 0;
    int y_ = 0;
    int z_ = 0;

    void genTexture(const std::vector<T>& data) {
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

        glTexImage3D(GL_TEXTURE_3D, 0, detail::InternalFormat<T>(), x_, y_, z_, 0,
                     detail::DataFormat<T>(), detail::DataType<T>(), data.data());
        glGenerateMipmap(GL_TEXTURE_3D);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

public:

    VolumeTexture() : Texture() {
        glGenTextures(1, &texture_id_);
    };

    ~VolumeTexture() override {
        if (texture_id_ >= 0) glDeleteTextures(1, &texture_id_);
    }

    VolumeTexture(const VolumeTexture&) = delete;
    VolumeTexture& operator=(const VolumeTexture&) = delete;

    VolumeTexture(VolumeTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        z_ = other.z_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    VolumeTexture& operator=(VolumeTexture&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        z_ = other.z_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    void setData(const std::vector<T>& data, int x, int y, int z) {
        x_ = x;
        y_ = y;
        z_ = z;
        assert((int)data.size() == x * y * z);
        genTexture(data);
    }

    void bind() const override {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
};

}  // namespace tomcat::gui

#endif // GUI_TEXTURES_H