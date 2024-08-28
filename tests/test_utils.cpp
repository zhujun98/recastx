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

using testing::ElementsAreArray;
using testing::Pointwise;
using testing::FloatNear;

namespace recastx::recon::utils::test {

TEST(TestUtils, TestExpandDataSize) {
    ASSERT_EQ(expandDataSize(63, 64), 64);
    ASSERT_EQ(expandDataSize(64, 64), 64);
    ASSERT_EQ(expandDataSize(65, 64), 128);
    
    ASSERT_EQ(expandDataSize(64, 128), 128);
}

TEST(TestUtils, TestComputeHistogram) {
    using Hist = std::pair<std::vector<float>, std::vector<float>>;

    constexpr float EPS_F = 1e-6;
    {
        Hist ret;
        computeHistogram<float>({1, 2, 3}, 1.f, 3.f, 4, ret);
        EXPECT_THAT(ret.first, Pointwise(FloatNear(EPS_F), {1.25f, 1.75f, 2.25f, 2.75f}));
        EXPECT_THAT(ret.second, Pointwise(FloatNear(EPS_F), {0.666667f, 0.f, 0.666667f, 0.666667f}));
    }

    {
        Hist ret;
        computeHistogram<float>({1, 1, 1, 1}, 1.f, 1.f, 4, ret);
        EXPECT_THAT(ret.first, Pointwise(FloatNear(EPS_F), {0.625f, 0.875f, 1.125f, 1.375f}));
        EXPECT_THAT(ret.second, Pointwise(FloatNear(EPS_F), {0.f, 0.f, 4.f, 0.f}));
    }

    {
        Hist ret;
        computeHistogram<float>({-5, -4, -3, 0, -2, -1, 1, 0, -1, 1, 0, 2, 3, 4, 5}, -2.5f, 1.5f, 3, ret);
        EXPECT_THAT(ret.first, Pointwise(FloatNear(EPS_F), {-1.833333f, -0.500000f, 0.833333f}));
        EXPECT_THAT(ret.second, Pointwise(FloatNear(EPS_F), {0.09375f, 0.46875f, 0.1875f}));
    }
}

} // namespace recastx::recon::utils::test