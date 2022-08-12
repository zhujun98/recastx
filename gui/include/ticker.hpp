#pragma once

namespace tomcat::gui {

class Ticker {
  public:
      virtual void tick(float time_elapsed) = 0;
};

} // tomcat::gui
