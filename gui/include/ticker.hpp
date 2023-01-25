#ifndef GUI_TICKER_H
#define GUI_TICKER_H

namespace tomcat::gui {

class Ticker {
  public:
      virtual void tick(double time_elapsed) = 0;
};

} // tomcat::gui

#endif // GUI_TICKER_H