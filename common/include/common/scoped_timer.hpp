/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef COMMON_SCOPEDTIMER_HPP
#define COMMON_SCOPEDTIMER_HPP

#include <chrono>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

class ScopedTimer {

public:
    using ClockType = std::chrono::steady_clock;

private:
    std::string topic_;
    std::string description_;
    const ClockType::time_point start_;

public:
    ScopedTimer(std::string topic, std::string description)
            : topic_(std::move(topic)), description_(std::move(description)), start_(ClockType::now()) {}

    ~ScopedTimer() {
        auto stop = ClockType::now();
        auto dt = std::chrono::duration_cast<std::chrono::microseconds>(stop - start_).count();

        spdlog::info("[{}] {} took {} ms", topic_, description_, dt / 1000.);
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;
};

#endif //COMMON_SCOPEDTIMER_HPP