#pragma once

namespace gui {

class InputHandler {
  public:
    virtual bool handleMouseButton(int /* button */, bool /* down */) { return false; }
    virtual bool handleScroll(double /* offset */) { return false; }
    virtual bool handleKey(int /* key */, bool /* down */, int /* mods */) { return false; }
    virtual bool handleChar(unsigned int /* c */) { return false; }
    // glfwGetCursorPos takes double as arguments
    virtual bool handleMouseMoved(double /* x */, double /* y */) { return false; }

    [[nodiscard]] virtual int priority() const { return 10; }
};

} // namespace gui
