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

using TextureId = GLuint;

struct TextureFormat {
    GLuint internal_format;
    GLenum format;
    GLenum type;
};

class Texture {

  protected:

    TextureId texture_ = 0;
    GLenum target_;
    GLenum texture_unit_ = GL_TEXTURE0;

    bool initialized_ = false;

  public:

    explicit Texture(GLenum target) : target_(target) {
    }

    Texture(TextureId texture_id, GLenum target)
            : texture_(texture_id), target_(target) {
    }

    virtual ~Texture() {
        glDeleteTextures(1, &texture_);
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept
            : texture_(std::exchange(other.texture_, -1)),
              target_(other.target_),
              texture_unit_(other.texture_unit_) {
    }

    Texture& operator=(Texture&& other) noexcept {
        texture_ = std::exchange(other.texture_, -1);
        target_ = other.target_;
        texture_unit_ = std::exchange(other.texture_unit_, GL_TEXTURE0);
        return *this;
    }

    void bind(unsigned int texture_unit = 0) {
        texture_unit_ = GL_TEXTURE0 + texture_unit;
        glActiveTexture(texture_unit_);
        glBindTexture(target_, texture_);
    }

    void unbind() const {
        glActiveTexture(texture_unit_);
        glBindTexture(target_, 0);
    }

    virtual void invalidate() { initialized_ = false; }

    [[nodiscard]] bool initialized() const { return initialized_; }
};

namespace details {

template<typename T>
inline TextureFormat getTextureFormat(int num_channels) {
    GLenum type;
    if constexpr (std::is_same_v<std::remove_const_t<T>, float>) {
        type = GL_FLOAT;
    } else if constexpr (std::is_same_v<std::remove_const_t<T>, uint16_t>) {
        type = GL_UNSIGNED_SHORT;
    } else if constexpr (std::is_same_v<std::remove_const_t<T>, char>
                         || std::is_same_v<std::remove_const_t<T>, unsigned char>) {
        type = GL_UNSIGNED_BYTE;
    } else {
        throw std::invalid_argument("Unsupported type");
    }

    if (num_channels == 1) {
        if (type == GL_UNSIGNED_SHORT) return { GL_R16UI, GL_RED_INTEGER, type };
        return { GL_R32F, GL_RED, type};
    }
    if (num_channels == 3) return { GL_RGB32F, GL_RGB, type };
    if (num_channels == 4) return { GL_RGBA32F, GL_RGBA, type };
    throw std::invalid_argument("Invalid number of channels");
}

} // namespace details

class Texture2D : public Texture {
    int x_ {0};
    int y_ {0};

    void createTexture() {
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    template<typename T>
    void initTexture(T* data, int x, int y, int num_channels) {
        glBindTexture(GL_TEXTURE_2D, texture_);
        auto fmt = details::getTextureFormat<T>(num_channels);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt.internal_format, x, y, 0, fmt.format, fmt.type, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void initUniformTexture(glm::vec4 color, int x, int y) {
        std::vector<unsigned char> data(4 * x * y * sizeof(unsigned char));
        for (int i = 0; i < x * y; ++i) {
            data[i * 4] = static_cast<unsigned char>(color[0] * 255.f);
            data[i * 4 + 1] = static_cast<unsigned char>(color[1] * 255.f);
            data[i * 4 + 2] = static_cast<unsigned char>(color[2] * 255.f);
            data[i * 4 + 3] = static_cast<unsigned char>(color[3] * 255.f);
        }

        initTexture(data.data(), x, y, 4);
    }

  public:

    Texture2D() : Texture(GL_TEXTURE_2D) {
        createTexture();
        x_ = 1;
        y_ = 1;
        initUniformTexture({0.f, 0.f, 0.f, 1.f}, x_, y_);
    }

    explicit Texture2D(const glm::vec3& color) : Texture(GL_TEXTURE_2D), x_(1), y_(1) {
        createTexture();
        initUniformTexture(glm::vec4(color, 1.f), x_, y_);
        initialized_ = true;
    }

    template<typename T>
    void setData(const T* data, int x, int y, int num_channels) {
        x_ = x;
        y_ = y;
        initTexture(data, x_, y_, num_channels);
        initialized_ = true;
    }
};


class Texture3D : public Texture {
    int x_ {0};
    int y_ {0};
    int z_ {0};

    void createTexture() {
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_3D, texture_);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    template<typename T>
    void initTexture(T* data, int x, int y, int z) {
        glBindTexture(GL_TEXTURE_3D, texture_);
        auto fmt = details::getTextureFormat<T>(1);
        glTexImage3D(GL_TEXTURE_3D, 0, fmt.internal_format, x, y, z, 0, fmt.format, fmt.type, data);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

  public:

    Texture3D() : Texture(GL_TEXTURE_3D) {
        createTexture();
        x_ = 1;
        y_ = 1;
        z_ = 1;
        std::vector<unsigned char> data(1);
        initTexture(data.data(), x_, y_, z_);
    }

    template<typename T>
    void setData(const T* data, int x, int y, int z) {
        x_ = x;
        y_ = y;
        z_ = z;
        initTexture(data, x_, y_, z_);
        initialized_ = true;
    }
};

}  // namespace recastx::gui

#endif // GUI_TEXTURES_H