/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/utils.hpp"

namespace recastx::recon::utils::test {

TEST(TestUtils, TestExpandDataSize) {
    ASSERT_EQ(expandDataSizeForGpu(63, 64), 64);
    ASSERT_EQ(expandDataSizeForGpu(64, 64), 64);
    ASSERT_EQ(expandDataSizeForGpu(65, 64), 128);
    
    ASSERT_EQ(expandDataSizeForGpu(64, 128), 128);
}

} // namespace recastx::recon::utils::test