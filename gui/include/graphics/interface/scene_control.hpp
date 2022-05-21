#pragma once

#include "window.hpp"


namespace gui {

class SceneList;

class SceneControl : public Window {
  public:
    SceneControl(SceneList& scenes);
    ~SceneControl();

    void describe() override;

  private:
    SceneList& scenes_;
};

} // namespace gui
