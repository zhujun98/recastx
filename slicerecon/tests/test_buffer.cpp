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

class MemoryBufferTest : public testing::Test {
  protected:

    size_t capacity_ = 2;
    size_t group_size_ = 4;
    size_t chunk_size_ = 3;

    MemoryBuffer<float> buffer_ = MemoryBuffer<float>();

    void SetUp() override {
        buffer_.initialize(capacity_, group_size_, chunk_size_);
    }
};

TEST_F(MemoryBufferTest, TestNormal) {

    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THROW(buffer_.ready(), std::out_of_range);

    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 0);
    ASSERT_EQ(buffer_.occupied(), 1);
    buffer_.fill<RawDtype>(_produceRawData({4, 5, 6}).data(), 0, 1);
    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 2);
    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, 3);
    EXPECT_EQ(&buffer_.ready(), &buffer_.back());
    buffer_.fetch();
    EXPECT_THAT(buffer_.front(), Pointwise(FloatNear(1e-6), 
                                           {1., 2., 3., 4., 5., 6., 1., 2., 3., 1., 2., 3.}));
    ASSERT_EQ(buffer_.occupied(), 0);
}

TEST_F(MemoryBufferTest, TestBufferFull) {
    for (size_t j = 0; j < group_size_; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1);

    for (size_t j = 0; j < group_size_; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({4, 5, 6}).data(), 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1); // group 0 was dropped
    EXPECT_THAT(buffer_.ready(), Pointwise(FloatNear(1e-6), 
                                           {4., 5., 6., 4., 5., 6., 4., 5., 6., 4., 5., 6.}));

    // group 1 was dropped; group 2 was added first and then dropped
    for (size_t j = 0; j < group_size_ - 1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({7, 8, 9}).data(), capacity_ + 2, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 2);

    for (size_t j = 0; j < group_size_-1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 4, 7}).data(), capacity_ + 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 2);

    buffer_.fill<RawDtype>(_produceRawData({9, 8, 7}).data(), capacity_ + 2, group_size_ - 1); 
    ASSERT_EQ(buffer_.occupied(), 1); // group 3 was dropped
    buffer_.fetch();
    EXPECT_THAT(buffer_.front(), Pointwise(FloatNear(1e-6), 
                                           {7., 8., 9., 7., 8., 9., 7., 8., 9., 9., 8., 7.}));
}

} // slicerecon::test