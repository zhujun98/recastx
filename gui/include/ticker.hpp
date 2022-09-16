#pragma once

namespace tomcat::gui {

class Ticker {
  public:
      virtual void tick(double time_elapsed) = 0;
};

} // tomcat::gui
