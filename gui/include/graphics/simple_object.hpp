/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_SIMPLE_OBJECT_H
#define GUI_SIMPLE_OBJECT_H

#include <vector>

#include "renderable.hpp"
#include "vertex.hpp"

namespace recastx::gui {

class SimpleObject : public Renderable {

  public:

    enum class Type { CUBE_FRAME, AXIS, AXIS_CUBE, SQUARE };
    enum class Mode { LINE, SURFACE };

    struct Model {
        int num_vertices {0};
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        Mode mode;
    };

  protected:

    static const char *vertex_shader;
    static const char *fragment_shader;

    GLuint VAO_;
    GLuint VBO_;
    GLuint EBO_;

    Model model_;
    glm::vec3 color_ {1.f, 1.f, 1.f};

    void init();

    SimpleObject();

  public:

    explicit SimpleObject(Type type);

    ~SimpleObject() override;

    void render(Renderer* renderer) override;

    void setColor(glm::vec3 color) { color_ = color; }
};

} // namespace recastx::gui

#endif // GUI_SIMPLE_OBJECT_H