/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUME_H
#define GUI_VOLUME_H

#include <array>
#include <memory>
#include <optional>

#include "style.hpp"
#include "textures.hpp"
#include "utils.hpp"

namespace recastx::gui {

class ShaderProgram;

class VolumeSlicer {

  public:

    class ShadowMap {

        GLuint vao_;
        GLuint vbo_;

        int width_;
        int height_;
        GLuint fbo_;

        GLuint light_texture_;
        GLuint eye_texture_;

        void createTexture(GLuint& texture_id) {
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, 0);
        }

    public:

        ShadowMap(int width, int height) : width_(width), height_(height) {

            const float screen_vertices[] = {
                    -1.0f,  1.0f,  0.0f, 1.0f,
                    -1.0f, -1.0f,  0.0f, 0.0f,
                     1.0f, -1.0f,  1.0f, 0.0f,
                     1.0f,  1.0f,  1.0f, 1.0f
            };

            glGenVertexArrays(1, &vao_);
            glBindVertexArray(vao_);

            glGenBuffers(1, &vbo_);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), &screen_vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glGenFramebuffers(1, &fbo_);

            glActiveTexture(GL_TEXTURE3);
            createTexture(light_texture_);
            createTexture(eye_texture_);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light_texture_, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, eye_texture_, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
                std::cout << "Framebuffer is not complete!\n";
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~ShadowMap() {
            glDeleteFramebuffers(1, &fbo_);
            glDeleteTextures(1, &light_texture_);
            glDeleteTextures(1, &eye_texture_);
        }

        void bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
            glViewport(0, 0, width_, height_);
        }

        friend class VolumeSlicer;
    };

    using DataType = std::vector<glm::vec3>;

  private:

    ShadowMap shadow_;

    size_t num_slices_;
    DataType slices_;
    float front_;

    GLuint vao_;
    GLuint vbo_;

    static constexpr int ids_[12] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5};

    static constexpr glm::vec3 vts_[8] = {
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f, -0.5f, -0.5f),
            glm::vec3( 0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f,  0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f, -0.5f,  0.5f),
            glm::vec3( 0.5f,  0.5f,  0.5f),
            glm::vec3(-0.5f,  0.5f,  0.5f)
    };

    static constexpr int egs_[12][2]= {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4}
    };

    static constexpr int pts_[8][12] = {
            { 0,1,5,6,   4,8,11,9,  3,7,2,10 }, { 0,4,3,11,  1,2,6,7,   5,9,8,10 },
            { 1,5,0,8,   2,3,7,4,   6,10,9,11}, { 7,11,10,8, 2,6,1,9,   3,0,4,5  },
            { 8,5,9,1,   11,10,7,6, 4,3,0,2  }, { 9,6,10,2,  8,11,4,7,  5,0,1,3  },
            { 9,8,5,4,   6,1,2,0,   10,7,11,3}, { 10,9,6,5,  7,2,3,1,   11,4,8,0 }
    };

    std::tuple<float, float, int> sortVertices(const glm::vec3& view_dir);

    void init();

  public:

    explicit VolumeSlicer(size_t num_slices);

    ~VolumeSlicer();

    void resize(size_t num_slices);

    void update(const glm::vec3& view_dir, bool inverted);

    void draw();

    void drawOnScreen();

    void drawOnBuffer(ShaderProgram* shadow_shader, ShaderProgram* volume_shader, bool is_view_inverted);

    void setFront(float front);
};

class Light;
class Viewport;

class Volume {

  public:

    using DataType = std::vector<float>;

  private:

    DataType data_;
    uint32_t x_;
    uint32_t y_;
    uint32_t z_;
    bool update_texture_ = false;
    VolumeTexture texture_;

    DataType buffer_;
    uint32_t b_x_;
    uint32_t b_y_;
    uint32_t b_z_;

    std::optional<std::array<float, 2>> min_max_vals_;

    RenderQuality volume_render_quality_;
    VolumeSlicer slicer_;
    bool global_illumination_ = true;
    float global_illumination_threshold_ = 0.1f;

    std::unique_ptr<ShaderProgram> shader_;
    std::unique_ptr<ShaderProgram> shadow_shader_;
    std::unique_ptr<ShaderProgram> screen_shader_;
    float alpha_ = 1.0f;

    void updateMinMaxVal();

public:

    Volume();

    bool init(uint32_t x, uint32_t y, uint32_t z);

    bool setShard(const std::string& shard, uint32_t pos);

    void preRender();

    void render(const glm::mat4& view,
                const glm::mat4& projection,
                float min_v,
                float max_v,
                const glm::vec3& view_dir,
                const glm::vec3& view_pos,
                const Light& light,
                const std::shared_ptr<Viewport>& vp);

    [[nodiscard]] bool hasTexture() const { return texture_.isReady(); }

    void clear();

    void clearBuffer();

    void bind() const { texture_.bind(); }

    void unbind() const { texture_.unbind(); }

    [[nodiscard]] const std::optional<std::array<float, 2>>& minMaxVals() const { return min_max_vals_; }

    [[nodiscard]] RenderQuality renderQuality() const { return volume_render_quality_; }

    void setRenderQuality(RenderQuality level);

    void setAlpha(float alpha) { alpha_ = alpha; }

    void setFront(float front) { slicer_.setFront(front); }

    [[nodiscard]] bool globalIllumination() const { return global_illumination_; }

    void setGlobalIllumination(bool state) { global_illumination_ = state; }

    [[nodiscard]] float globalIlluminationThreshold() const { return global_illumination_threshold_; }

    void setGlobalIlluminationThreshold(float value) { global_illumination_threshold_ = value; }
};


} // namespace recastx::gui

#endif // GUI_VOLUME_H
