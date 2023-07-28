/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/monitor.hpp"

namespace recastx::recon::test {

TEST(MonitorTest, TestGeneral) {
    const size_t monitor_projection_every = 5;
    Monitor mon(1, monitor_projection_every, 10);

    mon.countDark();
    mon.countFlat();

    for (size_t i = 0; i < monitor_projection_every - 1; ++i) {
        mon.countProjection(daq::Message(ProjectionType::PROJECTION, i, 2, 2, zmq::message_t()));
    }
    ASSERT_FALSE(mon.popProjection().has_value());

    mon.countProjection(daq::Message(ProjectionType::PROJECTION, 4, 2, 2, zmq::message_t()));
    ASSERT_TRUE(mon.popProjection().has_value());

    mon.addTomogram();

    mon.summarize();

    mon.reset();
    ASSERT_FALSE(mon.popProjection().has_value()); 
}

}