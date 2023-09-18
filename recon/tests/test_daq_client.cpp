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

#include "recon/daq/daq_factory.hpp"

namespace recastx::recon::test {

TEST(DaqClientTest, TestZmqClient) {
    std::string endpoint = "tcp://localhost:5555";
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "pull"));
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "pull", 2));
    EXPECT_NO_THROW(createDaqClient("default", endpoint, "sub"));
    EXPECT_THROW(createDaqClient("dlc", endpoint, "pull"), std::runtime_error);
    EXPECT_THROW(createDaqClient("default", endpoint, "push"), std::invalid_argument);
}

} // namespace recastx::recon::test