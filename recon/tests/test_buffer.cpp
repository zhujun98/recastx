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

#include "common/config.hpp"
#include "recon/buffer.hpp"

namespace recastx::recon::test {

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
    b2f.resize({3, 2});
    EXPECT_THAT(b2f.shape(), ElementsAre(3, 2));
    ASSERT_EQ(b2f.size(), 6);

    std::initializer_list<float> data1 {1.f, 2.f, 1.f, 2.f, 1.f, 2.f};
    std::initializer_list<float> data2 {3.f, 4.f, 3.f, 4.f, 3.f, 4.f};
    
    std::copy(data1.begin(), data1.end(), b2f.back().begin());
    b2f.prepare();
    EXPECT_THAT(b2f.ready(), Pointwise(FloatNear(1e-6), data1));
    ASSERT_TRUE(b2f.fetch(-1));
    EXPECT_THAT(b2f.front(), Pointwise(FloatNear(1e-6), data1));

    ASSERT_FALSE(b2f.fetch(0)); // test timeout
    ASSERT_FALSE(b2f.fetch(1)); // test timeout

    std::copy(data2.begin(), data2.end(), b2f.back().begin());
    ASSERT_FALSE(b2f.prepare());
    ASSERT_FALSE(b2f.tryPrepare(1));
    EXPECT_THAT(b2f.ready(), Pointwise(FloatNear(1e-6), data2));
    ASSERT_TRUE(b2f.prepare());
    b2f.fetch(0);
    ASSERT_TRUE(b2f.tryPrepare(1));
}


TEST(SliceBufferTest, TestNonOnDemand) {
    SliceBuffer<float> sbf;
    ASSERT_FALSE(sbf.onDemand());

    sbf.insert(1);
    ASSERT_EQ(sbf.size(), 1);
    std::array<size_t, 2> shape {3, 5};
    sbf.resize(shape);
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
    sbf.fetch(-1);
    for (auto& [k, slice] : sbf.front()) ASSERT_TRUE(std::get<0>(slice));
}

TEST(SliceBufferTest, TestOnDemand) {
    SliceBuffer<float, true> sbf;
    ASSERT_TRUE(sbf.onDemand());

    std::array<size_t, 2> shape {5, 6};
    sbf.resize(shape);
    sbf.insert(0);
    sbf.insert(1);
    sbf.insert(2);

    for (auto& [k, slice] : sbf.back()) {
        ASSERT_FALSE(std::get<0>(slice));
        ASSERT_EQ(std::get<2>(slice).shape(), shape);
        std::get<0>(slice) = true;
    }

    sbf.prepare();
    sbf.fetch(-1);
    for (auto& [k, slice] : sbf.front()) ASSERT_TRUE(std::get<0>(slice));

    sbf.prepare();
    sbf.fetch(-1);
    // "ready" status reset
    for (auto& [k, slice] : sbf.ready()) ASSERT_FALSE(std::get<0>(slice));
    for (auto& [k, slice] : sbf.front()) ASSERT_FALSE(std::get<0>(slice));
}

TEST(MemoryBufferTestUtils, TestCopyToBuffer) {
    {
        ProDtype dst[6];
        std::array<size_t, 2> dst_shape {2, 3};
        auto src = _produceRawData({1, 2, 3, 4, 5, 6});
        details::copyToBuffer<ProDtype, RawDtype>(dst, dst_shape, src.data(), {2, 3}, {1, 1});
        EXPECT_THAT(dst, Pointwise(FloatNear(1e-6), {1., 2., 3., 4., 5., 6.}));
    }
    {
        ProDtype dst[6];
        std::array<size_t, 2> dst_shape {2, 3};
        auto src = _produceRawData({1, 1, 2, 1, 3, 1,
                                                  1, 1, 2, 1, 3, 1,
                                                  4, 1, 5, 1, 6, 1,
                                                  4, 1, 5, 1, 6, 1});
        details::copyToBuffer<ProDtype, RawDtype>(dst, dst_shape, src.data(), {4, 6}, {2, 2});
        EXPECT_THAT(dst, Pointwise(FloatNear(1e-6), {1., 2., 3., 4., 5., 6.}));
    }
    {
        ProDtype dst[16] = {0.f};
        std::array<size_t, 2> dst_shape {4, 4};
        auto src = _produceRawData({1, 1, 2, 1, 3, 1,
                                                  1, 1, 2, 1, 3, 1,
                                                  4, 1, 5, 1, 6, 1,
                                                  4, 1, 5, 1, 6, 1});
        details::copyToBuffer<ProDtype, RawDtype>(dst, dst_shape, src.data(), {4, 6}, {2, 2});
        EXPECT_THAT(dst, Pointwise(FloatNear(1e-6), {0., 0., 0., 0.,
                                                     1., 2., 3., 0.,
                                                     4., 5., 6., 0.,
                                                     0., 0., 0., 0.}));
    }
    {
        ProDtype dst[20] = {0.f};
        std::array<size_t, 2> dst_shape {4, 5};
        auto src = _produceRawData({1, 1, 2, 1, 3, 1,
                                                  1, 1, 2, 1, 3, 1,
                                                  4, 1, 5, 1, 6, 1,
                                                  4, 1, 5, 1, 6, 1});
        details::copyToBuffer<ProDtype, RawDtype>(dst, dst_shape, src.data(), {4, 6}, {2, 2});
        EXPECT_THAT(dst, Pointwise(FloatNear(1e-6), {0., 0., 0., 0., 0.,
                                                     0., 1., 2., 3., 0.,
                                                     0., 4., 5., 6., 0.,
                                                     0., 0., 0., 0., 0.}));
    }
    {
        ProDtype dst[6];
        std::array<size_t, 2> dst_shape {2, 3};
        auto src = _produceRawData({6, 1, 5, 1, 4, 1, 1,
                                                  6, 1, 5, 1, 4, 1, 1,
                                                  3, 1, 2, 1, 1, 1, 1,
                                                  3, 1, 2, 1, 1, 1, 1,
                                                  1, 1, 1, 1, 1, 1, 1});
        details::copyToBuffer<ProDtype, RawDtype>(dst, dst_shape, src.data(), {5, 7}, {2, 2});
        EXPECT_THAT(dst, Pointwise(FloatNear(1e-6), {6., 5., 4., 3., 2., 1.}));
    }
}


class MemoryBufferTest : public testing::Test {

protected:

    int capacity_ = 3;
    std::array<size_t, 3> shape_ {4, 2, 3};

    MemoryBuffer<float, 3> buffer_ {capacity_};

    void SetUp() override {
        buffer_.resize(shape_);
    }
};

TEST_F(MemoryBufferTest, TestConstructor) {
    using Buffer = MemoryBuffer<float, 3>;
    EXPECT_THROW(Buffer(-1), std::runtime_error);
}


TEST_F(MemoryBufferTest, TestGeneral) {

    ASSERT_EQ(buffer_.capacity(), capacity_);

    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THROW(buffer_.ready(), std::out_of_range);

    buffer_.fill<RawDtype>(0, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
    ASSERT_EQ(buffer_.occupied(), 1);
    buffer_.fill<RawDtype>(1, _produceRawData({6, 5, 4, 3, 2, 1}).data(), {2, 3}, {1, 1});
    buffer_.fill<RawDtype>(2, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
    buffer_.fill<RawDtype>(3, _produceRawData({6, 5, 4, 3, 2, 1}).data(), {2, 3}, {1, 1});

    ASSERT_TRUE(buffer_.fetch(-1));
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
        buffer_.fill<RawDtype>(j, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 1);

    for (size_t j = 0; j < shape_[0]; ++j) {
        buffer_.fill<RawDtype>(4 + j, _produceRawData({6, 5, 4, 3, 2, 1}).data(), {2, 3}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 1); // group 0 was dropped
    EXPECT_THAT(buffer_.ready(),
                Pointwise(FloatNear(1e-6), {6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.,
                                            6., 5., 4., 3., 2., 1.}));

    // group 1 was dropped; group 2 was added first and then dropped
    for (size_t j = 0; j < shape_[0] - 1; ++j) {
        buffer_.fill<RawDtype>(4 * (capacity_ + 2) + j, _produceRawData({4, 5, 6, 7, 8, 9}).data(), {2, 3}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    for (size_t j = 0; j < shape_[0]-1; ++j) {
        buffer_.fill<RawDtype>(4 * (capacity_ + 1) + j, _produceRawData({1, 3, 5, 7, 9, 11}).data(), {2, 3}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 3);

    buffer_.fill<RawDtype>(4 * (capacity_ + 2) + shape_[0] - 1, _produceRawData({9, 8, 7, 6, 5, 4}).data(), {2, 3}, {1, 1});
    ASSERT_EQ(buffer_.occupied(), 1); // group 3 was dropped
    buffer_.fetch(-1);
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
            buffer_.fill<RawDtype>(j, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
        }
        if (i % 2 == 0) buffer_.fetch(-1);

        // Attempt to fill half of the second group.
        for (size_t j = 0; j < shape_[0] / 2; ++j) {
            buffer_.fill<RawDtype>(4 + j, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
        }
        if (i % 2 == 1) {
            buffer_.fetch(-1);
            ASSERT_EQ(buffer_.occupied(), 0);
        } else {
            ASSERT_EQ(buffer_.occupied(), 1);
        }
    }
}

TEST_F(MemoryBufferTest, TestReshape) {
    for (size_t j = 0; j < shape_[0]; ++j) {
        buffer_.fill<RawDtype>(j, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 1);
    EXPECT_THAT(buffer_.shape(), ElementsAre(4, 2, 3));
    ASSERT_EQ(buffer_.size(), 24);

    // expand
    std::array<size_t, 3> new_shape {4, 3, 4};
    buffer_.resize(new_shape);
    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THAT(buffer_.shape(), ElementsAre(4, 3, 4));
    ASSERT_EQ(buffer_.size(), 48);
    for (size_t j = 0; j < new_shape[0]; ++j) {
        buffer_.fill<RawDtype>(j, _produceRawData({1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6}).data(), {3, 4}, {1, 1});
    }
    ASSERT_EQ(buffer_.occupied(), 1);

    // shrink
    buffer_.resize(shape_);
    ASSERT_EQ(buffer_.occupied(), 0);
    EXPECT_THAT(buffer_.shape(), ElementsAre(4, 2, 3));
    ASSERT_EQ(buffer_.size(), 24);
}

TEST(MemoryBufferTest2, TestMWSR) {
    const size_t capacity = 600;
    const size_t chunk_size = 10;
    MemoryBuffer<float, 3> buffer {capacity};
    buffer.resize({chunk_size, 2, 3});

    auto w1 = std::thread([&] {
        for (int i = 0; i < chunk_size * capacity; i += 3) {
            buffer.fill<RawDtype>(i, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    auto w2 = std::thread([&] {
        for (int i = 1; i < chunk_size * capacity; i += 3) {
            buffer.fill<RawDtype>(i, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    auto w3 = std::thread([&] {
        for (int i = 2; i < chunk_size * capacity; i += 3) {
            buffer.fill<RawDtype>(i, _produceRawData({1, 2, 3, 4, 5, 6}).data(), {2, 3}, {1, 1});
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });

    int fetched = 0;
    auto r = std::thread([&] {
        for (int i = 0; i < capacity; ++i) {
            if (buffer.fetch(10)) {
                EXPECT_THAT(buffer.front(), ElementsAre(1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6,
                                                        1, 2, 3, 4, 5, 6));
                ++fetched;
            }
        }
    });
    w1.join();
    w2.join();
    w3.join();
    r.join();

    ASSERT_EQ(fetched, capacity);
    ASSERT_EQ(buffer.occupied(), 0);
    ASSERT_FALSE(buffer.isReady());
}

} // namespace recastx::recon::test