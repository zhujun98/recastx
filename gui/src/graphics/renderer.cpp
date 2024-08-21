/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/renderer.hpp"
#include "graphics/style.hpp"
#include "graphics/voxel_object.hpp"
#include "graphics/mesh_object.hpp"
#include "graphics/slice_object.hpp"
#include "graphics/image_object.hpp"
#include "graphics/simple_object.hpp"
#include "graphics/glyph_object.hpp"
#include "graphics/glyph_renderer.hpp"
#include "graphics/camera.hpp"

namespace recastx::gui {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Renderer::init(GLFWwindow* window) {
    glyph_renderer_ = std::make_unique<GlyphRenderer>();
    glyph_renderer_->init(96);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    ImPlot::CreateContext();

    Style::init();
}

void Renderer::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glEnable(GL_DEPTH_TEST);

    glClearColor(Style::BG_COLOR.x, Style::BG_COLOR.y, Style::BG_COLOR.z, Style::BG_COLOR.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::end() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::render(const std::vector<std::shared_ptr<VoxelObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) {
            ptr->render(this);
        }
    }
}

void Renderer::render(const std::vector<std::shared_ptr<MeshObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) {
            ptr->render(this);
        }
    }
}

void Renderer::render(const std::vector<std::shared_ptr<SliceObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) {
            ptr->render(this);
        }
    }
}

void Renderer::render(const std::vector<std::shared_ptr<ImageObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) {
            ptr->render(this);
        }
    }
}

void Renderer::render(const std::vector<std::shared_ptr<SimpleObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) ptr->render(this);
    }
}

void Renderer::render(const std::vector<std::shared_ptr<GlyphObject>>& objects) {
    for (auto ptr : objects) {
        if (ptr->visible()) ptr->render(this);
    }
}

void Renderer::useViewport(const Viewport& vp) {
    int pixel_width = int(vp.width * (float)width_);
    int pixel_height = int(vp.height * (float)height_);
    glViewport(int(vp.x * (float)width_), int(vp.y * (float)height_), pixel_width, pixel_height);

    aspect_ratio_ = (float)pixel_width / (float)pixel_height;
    if (vp.type == Viewport::Type::PERSPECTIVE) {
        float fov = glm::radians(45.f);
        proj_matrix_ = glm::perspective(fov, aspect_ratio_, 0.1f, 100.f);
    } else {
        proj_matrix_ = glm::ortho(-aspect_ratio_, aspect_ratio_, -1.f, 1.f, 0.1f, 100.f);
    }

    view_matrix_ = camera_->viewMatrix();
    view_pos_ = camera_->pos();
    view_dir_ = camera_->viewDir();

    vp_matrix_ =  proj_matrix_ * view_matrix_;

    prev_vp_ = std::make_unique<Viewport>(vp);
}

void Renderer::useViewport() {
    if (prev_vp_ != nullptr) useViewport(*prev_vp_);
}

void Renderer::onFramebufferSizeChanged(int width, int height) {
    width_ = width;
    height_ = height;
}

void Renderer::update(Camera* camera, Light* light) {
    camera_ = camera;
    light_ = light;

    if (light != nullptr) {
        glm::vec4 pos = glm::inverse(camera_->viewMatrix()) * glm::vec4{light_->relativePosition(), 1.f};
        light->setPosition(glm::vec3(pos));
    }
}

} // namespace recastx::gui