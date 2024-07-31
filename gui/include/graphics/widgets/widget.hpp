/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_WIDGET_h
#define GUI_WIDGET_h

#include <string>

#include <imgui.h>

#include "graphics/style.hpp"

namespace recastx::gui {

class Widget {

  protected:

    std::string name_;
    std::string id_;

  public:

    explicit Widget(std::string&& name) : name_(std::move(name)), id_(name_) {}

    virtual ~Widget() = default;

    void setName(std::string&& name) { name_ = std::move(name); }
    void setName(const std::string& name) { name_ = name; }

    virtual void draw() = 0;
};

} // namespace recastx::gui

#endif // GUI_WIDGET_h