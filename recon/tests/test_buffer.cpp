#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/config.hpp"
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

TEST(TripleTensorBufferTest, TestConstructor) {
    bool can_copy = std::is_copy_constructible_v<TripleTensorBuffer<float, 2>>;
    ASSERT_FALSE(can_copy);
    bool can_move = std::is_move_constructible_v<TripleTensorBuffer<float, 2>>;
    ASSERT_FALSE(can_move);
}

TEST(TripleTensorBufferTest, TestPrepareAndFetch) {
    TripleTensorBuffer<float, 2> b2f;
    b2f.reshape({3, 2});

    std::initializer_list<float> data1 {1.f, 2.f, 1.f, 2.f, 1.f, 2.f};
    std::initializer_list<float> data2 {3.f, 4.f, 3.f, 4.f, 3.f, 4.f};
    
    b2f.back() = data1;
    b2f.prepare();
    EXPECT_THAT(b2f.ready(), Pointwise(FloatNear(1e-6), data1));
    ASSERT_TRUE(b2f.fetch());
    EXPECT_THAT(b2f.front(), Pointwise(FloatNear(1e-6), data1));

    ASSERT_FALSE(b2f.fetch(0)); // test timeout
    ASSERT_FALSE(b2f.fetch(1)); // test timeout

    b2f.back() = data2;
    b2f.prepare();
    EXPECT_THAT(b2f.ready(), Pointwise(FloatNear(1e-6), data2));
}


TEST(SliceBufferTest, TestNonOnDemand) {
    SliceBuffer<float> sbf;
    sbf.insert(1);
    ASSERT_EQ(sbf.size(), 1);
    std::array<size_t, 2> shape {3, 5};
    sbf.reshape(shape);
    ASSERT_TRUE(sbf.insert(4));
    ASSERT_EQ(sbf.size(), 2);
    ASSERT_FALSE(sbf.insert(4));
    ASSERT_EQ(sbf.size(), 2);

    for (auto& [k, slice] : sbf.back()) {
        ASSERT_TRUE(std::get<0>(slice));
        ASSERT_EQ(std::get<2>(slice).shape(), shape);
    }
    ASSERT_EQ(sbf.shape(), shape);

    sbf.prepare();
    for (auto& [k, slice] : sbf.back()) ASSERT_TRUE(std::get<0>(slice));
    sbf.fetch();
    for (auto& [k, slice] : sbf.front()) ASSERT_TRUE(std::get<0>(slice));
}

TEST(SliceBufferTest, TestOnDemand) {
    SliceBuffer<float> sbf(true);
    std::array<size_t, 2> shape;
    sbf.reshape(shape);
    sbf.insert(0);
    sbf.insert(1);
    sbf.insert(2);

    for (auto& [k, slice] : sbf.back()) {
        ASSERT_FALSE(std::get<0>(slice));
        ASSERT_EQ(std::get<2>(slice).shape(), shape);
        std::get<0>(slice) = true;
    }

    sbf.prepare();
    sbf.fetch();
    for (auto& [k, slice] : sbf.front()) ASSERT_TRUE(std::get<0>(slice));

    sbf.prepare();
    sbf.fetch();
    // "ready" status reset
    for (auto& [k, slice] : sbf.ready()) ASSERT_FALSE(std::get<0>(slice));
    for (auto& [k, slice] : sbf.front()) ASSERT_FALSE(std::get<0>(slice));
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