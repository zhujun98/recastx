#ifndef GUI_TICKER_H
#define GUI_TICKER_H

namespace recastx::gui {

class Ticker {
  public:
      virtual void tick(double time_elapsed) = 0;
};

} // namespace recastx::gui

#endif // GUI_TICKER_H