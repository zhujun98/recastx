#pragma once

namespace tomcat::gui {

class Ticker {
  public:
      virtual void tick(double /* time_elapsed */);
};

} // tomcat::gui
