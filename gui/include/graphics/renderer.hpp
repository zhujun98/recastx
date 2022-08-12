#include <set>
#include <vector>

#include "graphics/render_target.hpp"
#include "ticker.hpp"

struct GLFWwindow;

namespace tomop::gui {

class Renderer {
    GLFWwindow* window_;

    struct RenderCompare {
      bool operator()(const RenderTarget* lhs, const RenderTarget* rhs) const {
        return lhs->zPriority() < rhs->zPriority();
      }
    };

    std::set<RenderTarget*, RenderCompare> targets_;
    std::vector<Ticker*> tickers_;

    double previous_time_;

  public:

    Renderer();
    ~Renderer();

    void spin();
    void register_target(RenderTarget& target);
    void register_ticker(Ticker& ticker);

    [[nodiscard]] GLFWwindow* window() const { return window_; }
};

} // tomop::gui
