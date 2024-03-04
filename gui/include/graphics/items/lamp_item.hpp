/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_LAMPITEM_H
#define GUI_LAMPITEM_H

#include <vector>

#include <glm/glm.hpp>

#include "graphics_item.hpp"
#include "graphics/light.hpp"
#include "graphics/shader_program.hpp"

namespace recastx::gui {

class LampItem : public GraphicsItem, public GraphicsGLItem {
    Light light_;

    bool show_;
    bool rel_pos_;
    float pos_[3];

    glm::vec3 color_;
    float ambient_;
    float diffuse_;
    float specular_;

    GLuint vao_;
    GLuint vbo_;
    std::unique_ptr<ShaderProgram> shader_;

    std::vector<float> vertices_;

    void genVertices(float size, int count);

    void init();

  public:

    explicit LampItem(Scene& scene);

    ~LampItem() override;

    void onWindowSizeChanged(int width, int height) override;

    void renderIm() override;

    void onFramebufferSizeChanged(int width, int height) override;

    void preRenderGl() override;

    void renderGl() override;

    [[nodiscard]] const Light& light() const { return light_; }
};

} // namespace recastx::gui

#endif // GUI_LAMPITEM_H