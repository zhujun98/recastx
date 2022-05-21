#pragma once


namespace gui {

class Window {
  public:
    Window();
    virtual ~Window() = 0;

    virtual void describe() = 0;
  private:
};

} // namespace gui
