#pragma once

#include <cstdio>

#include "ticker.hpp"

namespace tomop::gui {

class Recorder {
  public:
    Recorder();
    ~Recorder();

    void describe();
    void capture();

    void start();
    void stop();

  private:
    bool recording_ = false;
    FILE* ffmpeg_ = nullptr;
    int* buffer_ = nullptr;

    int width_ = 1024;
    int height_ = 768;
};

} // tomop::gui
