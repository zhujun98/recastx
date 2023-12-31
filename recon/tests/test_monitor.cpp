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

    mon.summarize(); // test with zero counts

    for (size_t i = 0; i < 3; ++i) mon.countDark();
    ASSERT_EQ(mon.numDarks(), 3);

    for (size_t i = 0; i < 4; ++i) mon.countFlat();
    ASSERT_EQ(mon.numFlats(), 4);

    for (size_t i = 0; i < 5; ++i) mon.countProjection();
    ASSERT_EQ(mon.numProjections(), 5);

    for (size_t i = 0; i < 6; ++i) mon.countTomogram();
    ASSERT_EQ(mon.numTomograms(), 6);

    // avoid NUMERICAL error
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    mon.countTomogram();

    mon.summarize();

    mon.reset();
}

TEST(MonitorTest, TestReset) {
    Monitor mon(2, 5);

    mon.countProjection();
    mon.countTomogram();
    ASSERT_EQ(mon.numProjections(), 1);
    ASSERT_EQ(mon.numTomograms(), 1);
    mon.countFlat();
    ASSERT_EQ(mon.numProjections(), 0);
    ASSERT_EQ(mon.numTomograms(), 0);
    ASSERT_EQ(mon.numFlats(), 1);

    mon.countProjection();
    mon.countTomogram();
    mon.countDark();
    ASSERT_EQ(mon.numProjections(), 0);
    ASSERT_EQ(mon.numTomograms(), 0);
    ASSERT_EQ(mon.numFlats(), 0);
    ASSERT_EQ(mon.numDarks(), 1);
}

} // namespace recastx::recon::test