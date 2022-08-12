#pragma once

namespace tomop::gui {

class Ticker {
  public:
      virtual void tick(float time_elapsed) = 0;
};

} // tomop::gui
