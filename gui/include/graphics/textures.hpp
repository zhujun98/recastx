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

template <typename T>
inline GLenum data_type();

template <>
inline GLenum data_type<uint8_t>() { return GL_UNSIGNED_BYTE; }

template <>
inline GLenum data_type<uint32_t>() { return GL_UNSIGNED_INT; }

template <>
inline GLenum data_type<float>() { return GL_FLOAT; }

template <typename T>
inline GLint format_type();

template <>
inline GLint format_type<uint8_t>() { return GL_RED; }

template <>
inline GLint format_type<uint32_t>() { return GL_RED; }

template <>
inline GLint format_type<float>() { return GL_R32F; }

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
class Texture2d : public Texture {

    int x_ = 0;
    int y_ = 0;

    void genTexture(const std::vector<T>& data) {
        // In reference to the hack in the 3D fill_texture below, we
        // have found that 1x1 textures are supported in intel
        // integrated graphics.
        glBindTexture(GL_TEXTURE_2D, texture_id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, format_type<T>(), x_, y_, 0, GL_RED,
                     data_type<T>(), data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

  public:

    Texture2d() : Texture() {
        glGenTextures(1, &texture_id_);
    };

    ~Texture2d() override {
        if (texture_id_ >= 0) glDeleteTextures(1, &texture_id_);
    }

    Texture2d(Texture2d&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    Texture2d& operator=(Texture2d&& other) noexcept {
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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

template <typename T = unsigned char>
class Texture3d  : public Texture {

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

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glTexImage3D(GL_TEXTURE_3D, 0, format_type<T>(), x_, y_, z_, 0, GL_RED,
                     data_type<T>(), data.data());
        glGenerateMipmap(GL_TEXTURE_3D);

        glBindTexture(GL_TEXTURE_3D, 0);
    }

  public:

    Texture3d() : Texture() {
        glGenTextures(1, &texture_id_);
    };

    ~Texture3d() override {
        if (texture_id_ >= 0) glDeleteTextures(1, &texture_id_);
    }

    Texture3d(const Texture3d&) = delete;
    Texture3d& operator=(const Texture3d&) = delete;

    Texture3d(Texture3d&& other) noexcept {
        x_ = other.x_;
        y_ = other.y_;
        z_ = other.z_;
        texture_id_ = other.texture_id_;
        other.texture_id_ = -1;
    }

    Texture3d& operator=(Texture3d&& other) noexcept {
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
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_3D, texture_id_);
    }

    void unbind() const override {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_3D, 0);
    }
};

}  // namespace tomcat::gui

#endif // GUI_TEXTURES_H