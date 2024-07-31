/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_INTERACTABLE_H
#define GUI_INTERACTABLE_H

#include "renderable.hpp"

namespace recastx::gui {

class Interactable : public Renderable {

  protected:

    bool hovering_ = false;
    bool dragging_ = false;

    bool selected_ = false;

  public:

    Interactable() = default;

    ~Interactable() override = default;

    void setHovering(bool state) { hovering_ = state; }

    void setDragging(bool state) { dragging_ = state; }
    [[nodiscard]] bool dragging() const { return dragging_; }

    void setSelected(bool state) { selected_ = state; }
};

} // recastx::gui

#endif // GUI_INTERACTABLE_H
