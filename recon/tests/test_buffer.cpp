#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tomcat/tomcat.hpp"
#include "recon/buffer.hpp"

namespace tomcat::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;
using ::testing::ElementsAre;

std::vector<char> _produceRawData(std::vector<RawDtype>&& data) {
    std::vector<char> raw(data.size() * 2, 0);
    memcpy(raw.data(), data.data(), raw.size());
    return raw;
}

class TripleVectorBufferTest : public testing::Test {

protected:

    std::array<size_t, 3> shape {2, 5, 4};

    TripleVectorBuffer<float, 3> buffer_;

    TripleVectorBufferTest() {
        buffer_.resize(shape);
    }

    ~TripleVectorBufferTest() override = default;
};

TEST_F(TripleVectorBufferTest, TestConstructors) {
    bool can_copy = std::is_copy_constructible_v<TripleVectorBuffer<float, 2>>;
    ASSERT_FALSE(can_copy);
    bool can_move = std::is_move_constructible_v<TripleVectorBuffer<float, 2>>;
    ASSERT_FALSE(can_move);
}

TEST_F(TripleVectorBufferTest, TestGeneral) {
    EXPECT_THAT(buffer_.shape(), ElementsAre(2, 5, 4));
    EXPECT_EQ(buffer_.chunkSize(), 2 * 5 * 4);

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


class SliceBufferTest : public testing::Test {

protected:

    const size_t capacity_;
    const std::array<size_t, 2> shape_;

    SliceBuffer<float> buffer_;

    SliceBufferTest() : capacity_(4), shape_ {3, 5}, buffer_{capacity_} {
        buffer_.resize(shape_);
    }
};

TEST_F(SliceBufferTest, TestGeneral) {
    ASSERT_EQ(buffer_.capacity(), capacity_);
    ASSERT_EQ(buffer_.chunkSize(), shape_[0] * shape_[1]);
    EXPECT_THAT(buffer_.shape(), ElementsAre(3, 5)); 
}


class MemoryBufferTest : public testing::Test {

protected:

    size_t capacity_ = 3;
    std::array<size_t, 3> shape_ {4, 2, 3};

    MemoryBuffer<float, 3> buffer_ {capacity_};

    void SetUp() override {
        buffer_.resize(shape_);
    }
};

TEST_F(MemoryBufferTest, TestGeneral) {

    ASSERT_EQ(buffer_.capacity(), capacity_);
    ASSERT_EQ(buffer_.chunkSize(), shape_[0] * shape_[1] * shape_[2]);

    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THROW(buffer_.ready(), std::out_of_range);

    buffer_.fill<RawDtype>(_produceRawData({1, 2, 3, 4, 5, 6}).data(), 0, 0);
    ASSERT_EQ(buffer_.occupied(), 1);
    buffer_.fill<RawDtype>(_produceRawData({6, 1, 5, 1, 4, 1, 
                                            3, 1, 2, 1, 1, 1}).data(), 0, 1, {2, 6}, {1, 2});
    buffer_.fill<RawDtype>(_produceRawData({1, 1, 2, 1, 3, 1, 
                                            1, 1, 2, 1, 3, 1,
                                            4, 1, 5, 1, 6, 1,
                                            4, 1, 5, 1, 6, 1}).data(), 0, 2, {4, 6}, {2, 2});
    buffer_.fill<RawDtype>(_produceRawData({6, 1, 5, 1, 4, 1, 1,
                                            6, 1, 5, 1, 4, 1, 1,
                                            3, 1, 2, 1, 1, 1, 1,
                                            3, 1, 2, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1}).data(), 0, 3, {5, 7}, {2, 2});
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
    for (size_t j = 0; j < shape_[0]; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 2, 3, 4, 5, 6}).data(), 0, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1);

    for (size_t j = 0; j < shape_[0]; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({6, 5, 4, 3, 2, 1}).data(), 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 1); // group 0 was dropped
    EXPECT_THAT(buffer_.ready(), 
                Pointwise(FloatNear(1e-6), {6., 5., 4., 3., 2., 1., 
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.}));

    // group 1 was dropped; group 2 was added first and then dropped
    for (size_t j = 0; j < shape_[0] - 1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({4, 5, 6, 7, 8, 9}).data(), capacity_ + 2, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    for (size_t j = 0; j < shape_[0]-1; ++j) {
        buffer_.fill<RawDtype>(_produceRawData({1, 3, 5, 7, 9, 11}).data(), capacity_ + 1, j); 
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    buffer_.fill<RawDtype>(_produceRawData({9, 8, 7, 6, 5, 4}).data(), capacity_ + 2, shape_[0] - 1); 
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
        for (size_t j = 0; j < shape_[0]; ++j) {
            buffer_.fill<RawDtype>(_produceRawData({1, 2, 3}).data(), 0, j); 
        }
        if (i % 2 == 0) buffer_.fetch();

        // Attempt to fill half of the second group.
        for (size_t j = 0; j < shape_[0] / 2; ++j) {
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