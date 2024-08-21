/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/light_object.hpp"
#include "graphics/renderer.hpp"

namespace recastx::gui {

LightObject::LightObject(std::shared_ptr<Light> light)
        : SimpleObject(), light_(std::move(light)) {
    setLightModel();
    init();
}

LightObject::~LightObject() = default;

void LightObject::render(Renderer* renderer) {
    if (!light_->visible()) return;

    pos_ = light_->position();

    shader_->use();
    shader_->setMat4("mvp", renderer->vpMatrix() * model());
    shader_->setBool("useVertexColor", false);
    shader_->setVec3("color", light_->color()); // the only difference from SimpleObject::render(Renderer*)

    glBindVertexArray(VAO_);
    glDrawElements(GL_LINES, model_.indices.size(), GL_UNSIGNED_INT, 0);
}

void LightObject::setLightModel() {
    model_.num_vertices = 6;
    model_.vertices = {
            -.1f,  .0f,  .0f,
             .1f,  .0f,  .0f,
             .0f, -.1f,  .0f,
             .0f,  .1f,  .0f,
             .0f,  .0f, -.1f,
             .0f,  .0f,  .1f
    };
    model_.indices = { 0, 1, 2, 3, 4, 5 };
    model_.mode = Mode::LINE;
}

} // namespace recastx::gui