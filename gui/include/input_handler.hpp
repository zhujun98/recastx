#pragma once

namespace gui {

class InputHandler {
  public:
    virtual bool handle_mouse_button(int /* button */, bool /* down */) { return false; }
    virtual bool handle_scroll(double /* offset */) { return false; }
    virtual bool handle_key(int /* key */, bool /* down */, int /* mods */) { return false; }
    virtual bool handle_char(unsigned int /* c */) { return false; }
    virtual bool handle_mouse_moved(float /* x */, float /* y */) { return false; }

    virtual int priority() const { return 10; }
};

} // namespace gui
