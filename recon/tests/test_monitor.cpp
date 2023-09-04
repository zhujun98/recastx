/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <chrono>
#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/monitor.hpp"

namespace recastx::recon::test {

TEST(MonitorTest, TestGeneral) {
    Monitor mon(1, 10);

    for (size_t i = 0; i < 3; ++i) mon.countDark();
    ASSERT_EQ(mon.numDarks(), 3);

    for (size_t i = 0; i < 4; ++i) mon.countFlat();
    ASSERT_EQ(mon.numFlats(), 4);

    for (size_t i = 0; i < 5; ++i) mon.countProjection();
    ASSERT_EQ(mon.numProjections(), 5);
    
    // avoid NUMERICAL error
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    mon.countTomogram();

    mon.summarize();

    mon.reset();
}

}