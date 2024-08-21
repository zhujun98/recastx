/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RENDERABLE_H
#define GUI_RENDERABLE_H

#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include "material.hpp"
#include "shader_program.hpp"

namespace recastx::gui {

class Renderer;

using ObjectId = unsigned int;

enum class RenderQuality { VERY_LOW = 1, LOW = 2, MEDIUM = 3, HIGH = 4, VERY_HIGH = 5 };

class Renderable {

    inline static ObjectId next_id_ = 0; // shared by all subclasses

  protected:

    ObjectId id_;

    glm::vec3 pos_ { 0.f, 0.f, 0.f };
    glm::mat4 rotation_;
    glm::vec3 scale_ { 1.f, 1.f, 1.f };

    MaterialID mat_id_;

    bool visible_ = true;

    std::unique_ptr<ShaderProgram> shader_;

  public:

    Renderable() : id_(next_id_++) {
    };

    virtual ~Renderable() = 0;

    virtual void render(Renderer* renderer) = 0;

    virtual void renderGUI() {};

    [[nodiscard]] ObjectId id() const { return id_; }

    virtual void setPosition(glm::vec3 pos) { pos_ = pos; }

    virtual void setRotation(glm::mat4 rotation) { rotation_ = rotation; }

    virtual void setScale(glm::vec3 scale) { scale_ = scale; }

    [[nodiscard]] glm::vec3 pos() const { return pos_; }

    [[nodiscard]] glm::mat4 model() const {
        auto mat = glm::scale(rotation_, scale_);
        mat = glm::translate(pos_) * mat;
        return mat;
    }

    void setMaterial(MaterialID id) { mat_id_ = id; }
    [[nodiscard]] MaterialID materialID() const { return mat_id_; }

    void setVisible(bool visible) { visible_ = visible; }
    [[nodiscard]] bool visible() const { return visible_; }
};

inline Renderable::~Renderable() {}

}  // namespace recastx::gui

#endif // GUI_RENDERABLE_H