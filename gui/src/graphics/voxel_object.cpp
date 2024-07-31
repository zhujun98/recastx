/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <algorithm>

#include "graphics/voxel_object.hpp"
#include "graphics/renderer.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/light_manager.hpp"
#include "graphics/widgets/widget.hpp"

namespace recastx::gui {

VoxelObject::Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height) {

    const float screen_vertices[] = {
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO_);
    glBindVertexArray(VAO_);

    glGenBuffers(1, &VBO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_vertices), &screen_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenFramebuffers(1, &FBO_);

    createTexture(light_texture_);
    createTexture(eye_texture_);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light_texture_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, eye_texture_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


VoxelObject::Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &FBO_);
    glDeleteTextures(1, &light_texture_);
    glDeleteTextures(1, &eye_texture_);
}

void VoxelObject::Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_);
    glViewport(0, 0, width_, height_);
}

void VoxelObject::Framebuffer::createTexture(GLuint &texture_id) {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, 0);
}

VoxelObject::Model::Model(size_t num_slices) : VolumeSlicer(num_slices) {
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    init();
}

VoxelObject::Model::~Model() {
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}

void VoxelObject::Model::setNumSlices(size_t num_slices) {
    if (resize(num_slices)) init();
}

void VoxelObject::Model::renderOnFramebuffer(Framebuffer *fb,
                                             ShaderProgram *vslice_shader,
                                             ShaderProgram *vlight_shader,
                                             bool is_view_inverted) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    fb->bind();

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBindVertexArray(VAO_);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, fb->light_texture_);

    for (int i = num_slices_; i >= 0; --i) {
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        vslice_shader->use();
        if (is_view_inverted) {
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        } else {
            glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
        }
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        vlight_shader->use();
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 12 * i, 12);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VoxelObject::Model::renderFramebufferOnScreen(Framebuffer *fb) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb->eye_texture_);

    glBindVertexArray(fb->VAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
}

void VoxelObject::Model::render() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * slices_.size(), slices_.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(VAO_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(slices_.size()));

    glDisable(GL_BLEND);
}

void VoxelObject::Model::init() {
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(glm::vec3) * slices_.size(),
                 0,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
}

VoxelObject::VoxelObject()
        : volume_shadow_(false),
          num_slices_(getNumSlices(RenderQuality::MEDIUM)),
          NUM_SLICES0(num_slices_),
          model_(new Model(num_slices_)),
          fb_(new Framebuffer(1200, 900)),
          threshold_(0.f) {

    auto vertex_shader =
#include "shaders/recon_vol.vert"
    ;
    auto fragment_shader =
#include "shaders/recon_vol.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);
    shader_->use();
    shader_->setInt("volumeTexture", 0);
    shader_->setInt("lutColor", 1);
    shader_->setInt("lutAlpha", 2);

    auto fb2screen_vertex_shader =
#include "shaders/recon_vol_fb2screen.vert"
    ;
    auto fb2screen_frag_shader =
#include "shaders/recon_vol_fb2screen.frag"
    ;

    vscreen_shader_ = std::make_unique<ShaderProgram>(fb2screen_vertex_shader, fb2screen_frag_shader);
    vscreen_shader_->use();
    vscreen_shader_->setInt("fbTexture", 0);

    auto vslice_vertex_shader =
#include "shaders/recon_vol_vslice.vert"
    ;
    auto vslice_frag_shader =
#include "shaders/recon_vol_vslice.frag"
    ;

    vslice_shader_ = std::make_unique<ShaderProgram>(vslice_vertex_shader, vslice_frag_shader);
    vslice_shader_->use();
    vslice_shader_->setInt("volumeTexture", 0);
    vslice_shader_->setInt("lutColor", 1);
    vslice_shader_->setInt("lutAlpha", 2);
    vslice_shader_->setInt("shadowTexture", 3);

    auto vlight_vertex_shader =
#include "shaders/recon_vol_vlight.vert"
    ;
    auto vlight_frag_shader =
#include "shaders/recon_vol_vlight.frag"
    ;

    vlight_shader_ = std::make_unique<ShaderProgram>(vlight_vertex_shader, vlight_frag_shader);
    vlight_shader_->use();
    vlight_shader_->setInt("volumeTexture", 0);
    vlight_shader_->setInt("lutAlpha", 2);
}

void VoxelObject::render(Renderer *renderer) {
    if (!intensity_.initialized()) return;

    if (volume_shadow_) {
        renderVolumeShadow(renderer);
    } else {
        renderSimple(renderer);
    }
}

void VoxelObject::renderGUI() {
    std::string id = "##Object" + std::to_string(id_);

    ImGui::SliderFloat(("Threshold" + id).c_str(), &threshold_, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

    if (ImGui::Checkbox(("Volume shadow" + id).c_str(), &volume_shadow_)) {
        if (volume_shadow_) {
//                view_front = 0.f;
//                 object->setFront(0.f);
        }
    }
}

void VoxelObject::setRenderQuality(recastx::gui::RenderQuality value) {
    setNumSlices(getNumSlices(value));
}

void VoxelObject::renderVolumeShadow(Renderer *renderer) {
    glm::vec3 target_pos = {0, 0, 0};
    glm::vec3 light_up = {0, 1, 0};
    const glm::mat4 light_projection = glm::perspective(45.f, 1.f, 0.1f, 100.f);
    const glm::mat4 bias = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0.5, 0.5, 0.5)),
                                      glm::vec3(0.5, 0.5, 0.5));

    auto light = renderer->lightManager()->lights()[0];
    auto [light_pos, light_dir] = light->geometry(target_pos);

    glm::vec3 light_vec = glm::normalize(-light_dir);

    glm::mat4 light_view = glm::lookAt(light_pos, target_pos, light_up);
    glm::mat4 light_mvp = light_projection * light_view;
    float sampling_rate = static_cast<float>(NUM_SLICES0) / static_cast<float>(num_slices_);

    auto mat = MaterialManager::instance().getMaterial<TransferFunc>(mat_id_);
    auto [min_v, max_v] = mat->minMaxVals();

    vslice_shader_->use();
    vslice_shader_->setMat4("mvp", renderer->vpMatrix());
    vslice_shader_->setMat4("mvpLightSpace", bias * light_mvp);
    vslice_shader_->setVec3("ambient", light->ambient());
    vslice_shader_->setVec3("diffuse", light->diffuse());
    vslice_shader_->setFloat("threshold", threshold_);
    vslice_shader_->setFloat("samplingRate", sampling_rate);
    vslice_shader_->setFloat("minValue", min_v);
    vslice_shader_->setFloat("maxValue", max_v);

    vlight_shader_->use();
    vlight_shader_->setMat4("mvpLight", light_mvp);
    vlight_shader_->setFloat("threshold", threshold_);
    vlight_shader_->setFloat("samplingRate", sampling_rate);
    vlight_shader_->setFloat("minValue", min_v);
    vlight_shader_->setFloat("maxValue", max_v);

    auto &view_dir = renderer->viewDir();
    bool is_view_inverted = glm::dot(view_dir, light_vec) < 0;
    const glm::vec3 &half_vec = glm::normalize((is_view_inverted ? -view_dir : view_dir) + light_vec);
    model_->update(half_vec);

    intensity_.bind(0);
    mat->bind(1, 2);
    model_->renderOnFramebuffer(fb_.get(), vslice_shader_.get(), vlight_shader_.get(), is_view_inverted);
    mat->unbind();
    intensity_.unbind();

    renderer->useViewport(VIEWPORT_MAIN);
    vscreen_shader_->use();
    model_->renderFramebufferOnScreen(fb_.get());
}

void VoxelObject::renderSimple(Renderer *renderer) {
    model_->update(renderer->viewDir());
    auto light = renderer->lightManager()->lights()[0];

    shader_->use();
    shader_->setMat4("mvp", renderer->vpMatrix());
    shader_->setVec3("ambient", light->ambient());
    shader_->setVec3("diffuse", light->diffuse());
    shader_->setFloat("threshold", threshold_);
    shader_->setFloat("samplingRate", static_cast<float>(NUM_SLICES0) / static_cast<float>(num_slices_));

    auto mat = MaterialManager::instance().getMaterial<TransferFunc>(mat_id_);
    auto [min_v, max_v] = mat->minMaxVals();
    shader_->setFloat("minValue", min_v);
    shader_->setFloat("maxValue", max_v);

    intensity_.bind(0);
    mat->bind(1, 2);
    model_->render();
    mat->unbind();
    intensity_.unbind();
}

int VoxelObject::getNumSlices(RenderQuality value) {
    if (value == RenderQuality::VERY_LOW) return 128;
    if (value == RenderQuality::LOW) return 256;
    if (value == RenderQuality::MEDIUM) return 512;
    if (value == RenderQuality::HIGH) return 1024;
    return 2048;
}

} // recastx::gui