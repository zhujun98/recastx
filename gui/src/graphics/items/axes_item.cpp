#include <iostream>

#include <imgui.h>

#include "graphics/items/axes_item.hpp"
#include "graphics/camera.hpp"
#include "graphics/scene.hpp"

namespace tomcat::gui {

AxesItem::AxesItem(Scene &scene) : StaticGraphicsItem(scene) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    static const GLfloat vertices[] = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // x
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // y
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f  // z
    };

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "../shaders/axes.vert"
    ;
    auto frag =
#include "../shaders/axes.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

AxesItem::~AxesItem() = default;

void AxesItem::renderIm() {
    ImGui::Checkbox("Show axes", &visible_);
}

void AxesItem::renderGl() {
    if (!visible_) return;

    // TODO draw axes on screen, should have access to camera here
    shader_->use();

    auto& camera = scene_.camera();
    shader_->setFloat("scale", camera.distance());
    shader_->setMat4("view", camera.matrix());
    shader_->setMat4("projection", scene_.projection());

    glBindVertexArray(vao_);
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 6);
}

} // tomcat::gui
