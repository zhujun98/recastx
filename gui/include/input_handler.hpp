#ifndef GUI_INPUTHANDLER_H
#define GUI_INPUTHANDLER_H

namespace recastx::gui {

class InputHandler {
  public:
    virtual bool handleMouseButton(int /* button */, int /* action */) { return false; }
    virtual bool handleScroll(float /* offset */) { return false; }
    virtual bool handleMouseMoved(float /* x */, float /* y */) { return false; }
    virtual bool handleKey(int /* key */, int /* action */, int /* mods */) { return false; }
    virtual bool handleChar(unsigned int /* c */) { return false; }
};

} // namespace recastx::gui

#endif // GUI_INPUTHANDLER_H
