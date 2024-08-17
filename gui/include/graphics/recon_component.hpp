/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_RECONITEM_H
#define GUI_RECONITEM_H

#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

#include "graphics/light.hpp"
#include "graphics/material.hpp"
#include "graphics/renderable.hpp"
#include "graphics/scene.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/style.hpp"
#include "graphics/textures.hpp"
#include "fps_counter.hpp"

namespace recastx::gui {

class ReconComponent {

    enum class DragType : int { none, rotator, translator};

    GLuint rotation_axis_vao_;
    GLuint rotation_axis_vbo_;

    bool auto_levels_ = true;
    bool clamp_negatives_ = true;
    bool update_min_max_val_ = false;
    float min_val_;
    float max_val_;

    glm::mat4 matrix_;

    [[nodiscard]] std::vector<Slice*> sortedSlices() const;

    void maybeSwitchDragMachine(DragType type);

    void updateMinMaxValues();

public:

    ReconComponent();

    ~ReconComponent();

    void preRender();
};

} // namespace recastx::gui

#endif // GUI_RECONITEM_H