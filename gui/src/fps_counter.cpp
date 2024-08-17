#include "fps_counter.hpp"

namespace recastx::gui {

FpsCounter::FpsCounter() : prev_(glfwGetTime()), threshold_(1.) {}

FpsCounter::~FpsCounter() = default;

void FpsCounter::count() {
    ++frames_;
    double curr = glfwGetTime();
    if (curr - prev_ > threshold_) {
        fps_ = frames_ / (curr - prev_);
        prev_ = curr;
        frames_ = 0;
    }
}

[[nodiscard]] double FpsCounter::frameRate() {
    if (glfwGetTime() - prev_ > FpsCounter::reset_interval_) fps_ = 0.;
    return fps_;
}

} // namespace recastx::gui
