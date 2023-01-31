#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tomcat/tomcat.hpp"
#include "recon/buffer.hpp"

namespace tomcat::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

std::vector<char> _produceRawData(std::vector<RawDtype>&& data) {
    std::vector<char> raw(data.size() * 2, 0);
    memcpy(raw.data(), data.data(), raw.size());
    return raw;
}

class TrippleBufferTest : public testing::Test {
  protected:

    size_t capacity_ = 2;
    std::array<size_t, 2> shape {5, 4};

    TripleVectorBuffer<float, 2> buffer_;

    void SetUp() override {
        buffer_.resize(capacity_, shape);
    }
};

TEST_F(TrippleBufferTest, TestConstructors) {
    bool can_copy = std::is_copy_constructible_v<TripleVectorBuffer<float, 2>>;
    ASSERT_FALSE(can_copy);
    bool can_move = std::is_move_constructible_v<TripleVectorBuffer<float, 2>>;
    ASSERT_FALSE(can_move);
}

TEST_F(TrippleBufferTest, TestGeneral) {

    ASSERT_EQ(buffer_.capacity(), capacity_);

    std::vector<float> data1 {1.f, 2.f};
    std::vector<float> data2 {3.f, 4.f};
    std::vector<float> data3 {5.f, 6.f};
    
    buffer_.back() = data1;
    buffer_.prepare();
    EXPECT_THAT(buffer_.ready(), Pointwise(FloatNear(1e-6), data1));
    ASSERT_TRUE(buffer_.fetch());
    EXPECT_THAT(buffer_.front(), Pointwise(FloatNear(1e-6), data1));

    ASSERT_FALSE(buffer_.fetch(0)); // test timeout
    ASSERT_FALSE(buffer_.fetch(1)); // test timeout

    buffer_.back() = data2;
    buffer_.prepare();

    buffer_.back() = data3;
}

class MemoryBufferTest : public testing::Test {
  protected:

    size_t capacity_ = 3;
    size_t group_size_ = 4;
    std::array<size_t, 2> shape_ {2, 3};

    MemoryBuffer<float, 2> buffer_;

    void SetUp() override {
        buffer_.resize(capacity_, group_size_, shape_);
    }
};

TEST_F(MemoryBufferTest, TestGeneral) {

    ASSERT_EQ(buffer_.capacity(), capacity_);
    ASSERT_EQ(buffer_.groupSize(), group_size_);
    ASSERT_EQ(buffer_.chunkSize(), shape_[0] * shape_[1]);

    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THROW(buffer_.ready(), std::out_of_range);

    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3, 4, 5, 6}).data(), 0, 0);
    ASSERT_EQ(buffer_.occupied(), 1);
    buffer_.fill<RawDtype>(_produceRawData({6, 5, 4, 3, 2, 1}).data(), 0, 1);
    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3, 4, 5, 6}).data(), 0, 2);
    buffer_.fill<RawDtype>(_produceRawData({6, 5, 4, 3, 2, 1}).data(), 0, 3);
    EXPECT_EQ(&buffer_.ready(), &buffer_.back());
    ASSERT_TRUE(buffer_.fetch());
    EXPECT_THAT(buffer_.front(), 
                Pointwise(FloatNear(1e-6), {1., 2., 3., 4., 5., 6., 
                                            6., 5., 4., 3., 2., 1.,
                                            1., 2., 3., 4., 5., 6.,
                                            6., 5., 4., 3., 2., 1.}));
    ASSERT_EQ(buffer_.occupied(), 0);
    ASSERT_FALSE(buffer_.fetch(10));
}

TEST_F(MemoryBufferTest, TestBufferFull) {
    for (size_t j = 0; j < group_size_; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 2, 3, 4, 5, 6}).data(), 0, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1);

    for (size_t j = 0; j < group_size_; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({6, 5, 4, 3, 2, 1}).data(), 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1); // group 0 was dropped
    EXPECT_THAT(buffer_.ready(), 
                Pointwise(FloatNear(1e-6), {6., 5., 4., 3., 2., 1., 
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.}));

    // group 1 was dropped; group 2 was added first and then dropped
    for (size_t j = 0; j < group_size_ - 1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({4, 5, 6, 7, 8, 9}).data(), capacity_ + 2, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    for (size_t j = 0; j < group_size_-1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 3, 5, 7, 9, 11}).data(), capacity_ + 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    buffer_.fill<RawDtype>(_produceRawData({9, 8, 7, 6, 5, 4}).data(), capacity_ + 2, group_size_ - 1); 
    ASSERT_EQ(buffer_.occupied(), 1); // group 3 was dropped
    buffer_.fetch();
    EXPECT_THAT(buffer_.front(), 
                Pointwise(FloatNear(1e-6), {4., 5., 6., 7., 8., 9., 
                                            4., 5., 6., 7., 8., 9.,
                                            4., 5., 6., 7., 8., 9.,
                                            9., 8., 7., 6., 5., 4.}));
}

TEST_F(MemoryBufferTest, TestSameDataReceivedRepeatedly) {
    for (size_t i = 0; i < 8; ++i) {
        // Attempt to fill the 1st group.
        for (size_t j = 0; j < group_size_; ++j) {
            buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, j); 
        }
        if (i % 2 == 0) buffer_.fetch();

        // Attempt to fill half of the second group.
        for (size_t j = 0; j < group_size_ / 2; ++j) {
            buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 1, j); 
        }
        if (i % 2 == 1) {
            buffer_.fetch();
            ASSERT_EQ(buffer_.occupied(), 0);
        } else {
            ASSERT_EQ(buffer_.occupied(), 1);
        }
    }
}

} // tomcat::recon::test