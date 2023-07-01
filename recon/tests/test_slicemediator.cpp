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

#include "recon/slice_mediator.hpp"

namespace recastx::recon::test {

TEST(SliceMediatorTest, TestUpdate) {
    SliceMediator mediator;
    auto& all = mediator.allSlices();
    ASSERT_FALSE(all.onDemand());
    auto& on_demand = mediator.onDemandSlices();
    ASSERT_TRUE(on_demand.onDemand());
    auto& params = mediator.params();

    ASSERT_EQ(all.size(), 0);
    ASSERT_EQ(on_demand.size(), 0);
    ASSERT_EQ(params.size(), 0);

    mediator.update(1, Orientation());
    ASSERT_EQ(all.size(), 1);
    ASSERT_EQ(on_demand.size(), 1);
    ASSERT_EQ(params.size(), 1);

    Orientation orient1 {1, 1, 1, 1, 1, 1, 1, 1, 1};
    mediator.update(1 + MAX_NUM_SLICES, orient1);
    ASSERT_EQ(all.size(), 1);
    ASSERT_EQ(on_demand.size(), 1);
    ASSERT_EQ(params.size(), 1);
    ASSERT_EQ(params.at(1).second, orient1);

    std::array<size_t, 2> shape {5, 6};
    mediator.resize(shape);
    ASSERT_EQ(all.shape(), shape);
    ASSERT_EQ(on_demand.shape(), shape);

    Orientation orient2 {2, 2, 2, 2, 2, 2, 2, 2, 2};
    mediator.update(0, orient2);
    ASSERT_EQ(all.size(), 2);
    ASSERT_EQ(on_demand.size(), 2);
    ASSERT_EQ(params.size(), 2);
    ASSERT_EQ(params.at(0).second, orient2);
}

} // namespace recastx::recon::test