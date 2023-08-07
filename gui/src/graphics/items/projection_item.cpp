/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <imgui.h>

#include "graphics/items/projection_item.hpp"
#include "graphics/aesthetics.hpp"
#include "graphics/framebuffer.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"
#include "logger.hpp"

namespace recastx::gui {

ProjectionItem::ProjectionItem(Scene& scene)
        : GraphicsItem(scene),
          img_(0),
          fb_(new Framebuffer),
          cm_(new Colormap) {
    scene.addItem(this);

    static constexpr float s = 1.0f;
    static constexpr GLfloat square[] = {
        -s, -s, 0.0f, 0.0f, 0.0f,
        -s,  s, 0.0f, 0.0f, 1.0f,
         s,  s, 0.0f, 1.0f, 1.0f,
         s, -s, 0.0f, 1.0f, 0.0f};

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "../shaders/projection.vert"
    ;
    auto frag =
#include "../shaders/projection.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);

    cm_->set(ImPlotColormap_Greys);
}

ProjectionItem::~ProjectionItem() = default;

void ProjectionItem::onWindowSizeChanged(int width, int height) {
    size_ = {
            Style::PROJECTION_WIDTH * (float)width,
            Style::PROJECTION_HEIGHT * (float)height
    };

    pos_ = {
            (1.0f - Style::MARGIN - Style::PROJECTION_WIDTH) * (float)width,
            (1.0f - Style::PROJECTION_HEIGHT - Style::STATUS_BAR_HEIGHT - 2.f * Style::MARGIN) * (float)(height)
    };
}

void ProjectionItem::renderIm() {
    ImGui::Checkbox("Show projection", &visible_);

    if (visible_) {
        ImGui::SetNextWindowPos(pos_);
        ImGui::SetNextWindowSize(size_);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding_, padding_));
        ImGui::Begin("Raw projection", NULL, ImGuiWindowFlags_NoDecoration);

        ImGui::Image((void*)(intptr_t)fb_->texture(), ImVec2(fb_->width(), fb_->height()));

        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void ProjectionItem::onFramebufferSizeChanged(int width, int height) {
    int w = static_cast<int>(Style::PROJECTION_WIDTH * width) - 2 * padding_;
    int h = static_cast<int>(Style::PROJECTION_HEIGHT * height) - 2 * padding_;

    fb_->prepareForRescale(w, h);
}

void ProjectionItem::renderGl() {
    shader_->use();
    shader_->setInt("colormap", 0);
    shader_->setInt("projectionTexture", 1);

    fb_->bind();
    cm_->bind();
    img_.bind();
    glBindVertexArray(vao_);
    glViewport(0, 0, fb_->width(), fb_->height());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    img_.unbind();
    cm_->unbind();
    fb_->unbind();

    scene_.useViewport();
}

bool ProjectionItem::updateServerParams() {
    return false;
}

bool ProjectionItem::consume(const DataType& packet) {
    if (std::holds_alternative<rpc::ProjectionData>(packet)) {
        const auto& data = std::get<rpc::ProjectionData>(packet);

        setProjectionData(data.data(), {data.col_count(), data.row_count()});
        log::info("Set projection data");
        return true;
    }

    return false;
}

void ProjectionItem::setProjectionData(const std::string &data, const std::array<uint32_t, 2> &size) {
    Projection::DataType proj(size[0] * size[1]);
    std::memcpy(proj.data(), data.data(), data.size());
    assert(data.size() == proj.size() * sizeof(Projection::DataType::value_type));
    img_.setData(std::move(proj), {size[0], size[1]});
}

} // namespace recastx::gui
