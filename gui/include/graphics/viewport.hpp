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

struct Viewport {

    enum class Type { ORTHO, PERSPECTIVE };

    float x;
    float y;
    float width;
    float height;
    Type type;
};

#endif // GUI_VIEWPORT_H