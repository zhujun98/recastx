/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VIEWPORT_H
#define GUI_VIEWPORT_H

#include <optional>

#include <glm/glm.hpp>

namespace recastx::gui {

class Viewport {

protected:

    int x_ = 0;
    int y_ = 0;
    int w_ = 1;
    int h_ = 1;
    float asp_ = 1.f;

    float fov_ = glm::radians(45.f);
    bool perspective_;
    float near_;
    float far_;

    std::optional<glm::mat4> projection_;

public:

    Viewport(bool perspective = true, float near = 0.1f, float far = 50.f);

    virtual ~Viewport();

    void update(int x, int y, int w, int h);

    [[nodiscard]] float aspectRatio() const { return asp_; }

    [[nodiscard]] const glm::mat4& projection();

    void use() const;
};

} // namespace recastx::gui

#endif //GUI_VIEWPORT_H