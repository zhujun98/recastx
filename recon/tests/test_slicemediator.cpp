#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/slice_mediator.hpp"

namespace tomcat::recon::test {

TEST(SliceMediatorTest, TestUpdate) {
    SliceMediator mediator;
    auto& all = mediator.allSlices();
    auto& on_demand = mediator.onDemandSlices();
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
    mediator.reshape(shape);
    ASSERT_EQ(all.shape(), shape);
    ASSERT_EQ(on_demand.shape(), shape);

    Orientation orient2 {2, 2, 2, 2, 2, 2, 2, 2, 2};
    mediator.update(0, orient2);
    ASSERT_EQ(all.size(), 2);
    ASSERT_EQ(on_demand.size(), 2);
    ASSERT_EQ(params.size(), 2);
    ASSERT_EQ(params.at(0).second, orient2);
}

} // tomcat::recon::test