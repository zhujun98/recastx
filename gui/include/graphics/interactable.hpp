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
#include "event.hpp"

namespace recastx::gui {

class Interactable : public Renderable {

  protected:

    bool highlighting_ = false;

    bool hovering_ = false;
    bool dragging_ = false;

    bool selected_ = false;

  public:

    Interactable() = default;

    ~Interactable() override = default;

    void setHighlighting(bool state) { highlighting_ = state; }
    [[nodiscard]] bool isHighlighting() const { return highlighting_; }

    void setHovering(bool state) { hovering_ = state; }
    [[nodiscard]] bool isHovering() const { return hovering_; }

    void setDragging(bool state) { dragging_ = state; }
    [[nodiscard]] bool isDragging() const { return dragging_; }

    void setSelected(bool state) { selected_ = state; }
    [[nodiscard]] bool isSelected() const { return selected_; }

    virtual bool mouseHoverEvent(const MouseHoverEvent&) { return false; }
    virtual bool mouseDragEvent(const MouseDragEvent&) { return false; }
};

} // recastx::gui

#endif // GUI_INTERACTABLE_H
