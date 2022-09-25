#include <iostream>

#include <imgui.h>

#include "graphics/items/axiscube_item.hpp"
#include "graphics/camera3d.hpp"
#include "graphics/primitives.hpp"
#include "graphics/scene.hpp"

namespace tomcat::gui {

AxiscubeItem::AxiscubeItem(Scene &scene) : GraphicsItem(scene) {
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitives::axiscube), primitives::axiscube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    auto vert =
#include "../shaders/simple_cube.vert"
    ;
    auto frag =
#include "../shaders/simple_cube.frag"
    ;

    shader_ = std::make_unique<ShaderProgram>(vert, frag);
}

AxiscubeItem::~AxiscubeItem() = default;

void AxiscubeItem::renderIm() {}

void AxiscubeItem::renderGl(const glm::mat4& view,
                        const glm::mat4& projection,
                        const RenderParams& /*params*/) {

    shader_->use();

    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDisable(GL_DEPTH_TEST);
}

} // tomcat::gui
