#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slicerecon/data_types.hpp"
#include "slicerecon/buffer.hpp"

namespace slicerecon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

std::vector<char> _produceRawData(std::vector<RawDtype>&& data) {
    std::vector<char> raw(data.size() * 2, 0);
    memcpy(raw.data(), data.data(), raw.size());
    return raw;
}

TEST(TestMemoryBuffer, TestNormal) {
    size_t capacity = 2;
    size_t group_size = 4;
    size_t chunk_size = 3;

    auto buffer = MemoryBuffer<float>();
    buffer.initialize(capacity, group_size, chunk_size);

    ASSERT_EQ(buffer.occupied(), 0);
    EXPECT_THROW(buffer.ready(), std::out_of_range);

    buffer.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 0);
    ASSERT_EQ(buffer.occupied(), 1);
    buffer.fill<RawDtype>(_produceRawData({4, 5, 6}).data(), 0, 1);
    buffer.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 2);
    ASSERT_FALSE(buffer.isReady());
    buffer.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 3);
    ASSERT_TRUE(buffer.isReady());
    buffer.fetch();
    EXPECT_THAT(buffer.front(), Pointwise(FloatNear(1e-6), 
                                          {1., 2., 3., 4., 5., 6., 1., 2., 3., 1., 2., 3.}));
    ASSERT_EQ(buffer.occupied(), 0);

    buffer.fill<RawDtype>(_produceRawData({7, 8, 9}).data(), 1, 2);
    ASSERT_EQ(buffer.occupied(), 1);
    ASSERT_FALSE(buffer.isReady());

    ASSERT_EQ(buffer.occupied(), 1); 
    ASSERT_FALSE(buffer.isReady());
    EXPECT_THAT(buffer.ready(), Pointwise(FloatNear(1e-6), 
                                          {0., 0., 0., 0., 0., 0., 7., 8., 9., 0., 0., 0.}));
    EXPECT_EQ(&buffer.ready(), &buffer.back());
}

} // slicerecon::test