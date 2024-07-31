/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_IMAGE_OBJECT_H
#define GUI_IMAGE_OBJECT_H

#include <vector>

#include "renderable.hpp"
#include "common/config.hpp"

namespace recastx::gui {

class Renderer;

class ImageObject : public Renderable {

    GLuint VAO_;
    GLuint VBO_;

    static constexpr float vertices_[] = {
            -1.f, -1.f, 0.0f,
            -1.f,  1.f, 0.0f,
             1.f,  1.f, 0.0f,
             1.f, -1.f, 0.0f
    };

    Texture2D intensity_;
    float aspect_ratio_ { 1.f };
    bool keep_aspect_ratio_ { true };

    void init();

  public:

    ImageObject();

    ~ImageObject() override;

    void render(Renderer* renderer) override;

    void renderGUI();

    template<typename T>
    void setIntensity(const T* data, uint32_t x, uint32_t y) {
        intensity_.setData(data, x, y, 1);
        aspect_ratio_ = (float) y / (float)x;
    }

    void resetIntensity() {
        intensity_.invalidate();
    }
};

} // recastx::gui

#endif // GUI_IMAGE_OBJECT_H
