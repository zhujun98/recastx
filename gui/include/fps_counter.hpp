/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace recastx::gui {

class FpsCounter {

    int frames_ = 0;
    double prev_;
    double fps_ = 0.;

    double threshold_;
    static constexpr double reset_interval_ = 5.;

public:

    FpsCounter();
    ~FpsCounter();

    void count();

    [[nodiscard]] double frameRate();
};

} // namespace recastx::gui

#endif //GUI_UTILS_H