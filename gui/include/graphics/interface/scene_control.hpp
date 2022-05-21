#pragma once

#include "window.hpp"


namespace gui {

class SceneList;

class SceneControl : public Window {
  public:
    explicit SceneControl(SceneList& scenes);
    ~SceneControl() override;

    void describe() override;

  private:
    SceneList& scenes_;
};

} // namespace gui
