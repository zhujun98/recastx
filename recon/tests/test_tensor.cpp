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

#include <algorithm>

#include "recon/tensor.hpp"

namespace recastx::recon::test {

using ::testing::ElementsAre;

TEST(TestTensor, TestConstructor) {
    {
        Tensor<float, 1> tf({3});
        ASSERT_THAT(tf.shape(), ElementsAre(3));
        ASSERT_EQ(tf.size(), 3);
    }
    {
        Tensor<float, 2> tf({2, 4});
        ASSERT_THAT(tf.shape(), ElementsAre(2, 4));
        ASSERT_EQ(tf.size(), 8);
    }
    {
        using TS = Tensor<float, 2>;
        EXPECT_THROW(TS({2, 2}, {1, 2, 3}), std::runtime_error);

        const std::vector<float> data {1, 2, 3};
        EXPECT_THROW(TS({2, 2}, data), std::runtime_error);
    }
    {
        const std::vector<float> data {1, 2, 3, 4};
        Tensor<float, 2> tf({2, 2}, data);
        ASSERT_THAT(tf.shape(), ElementsAre(2, 2));
        ASSERT_THAT(tf, ElementsAre(1, 2, 3, 4));
        ASSERT_THAT(data, ElementsAre(1, 2, 3, 4));
    }
    {
        std::vector<float> data {1, 2, 3, 4};
        Tensor<float, 2> tf({2, 2}, std::move(data));
        ASSERT_THAT(tf.shape(), ElementsAre(2, 2));
        ASSERT_THAT(tf, ElementsAre(1, 2, 3, 4));
        ASSERT_TRUE(data.empty());
    }
    {
        Tensor<float, 2> tf({3, 2}, {1, 1, 1, 2, 2, 2});
        ASSERT_THAT(tf.shape(), ElementsAre(3, 2));
        ASSERT_THAT(tf.size(), 6);

        Tensor<float, 2> tf_c(tf);
        ASSERT_THAT(tf_c.shape(), ElementsAre(3, 2));
        ASSERT_THAT(tf_c, ElementsAre(1, 1, 1, 2, 2, 2));

        Tensor<float, 2> tf_c2;
        tf_c2 = tf;
        ASSERT_THAT(tf_c2.shape(), ElementsAre(3, 2));
        ASSERT_THAT(tf_c2, ElementsAre(1, 1, 1, 2, 2, 2));

        Tensor<float, 2> tf_m(std::move(tf));
        ASSERT_THAT(tf_m.shape(), ElementsAre(3, 2));
        ASSERT_THAT(tf_m, ElementsAre(1, 1, 1, 2, 2, 2));
        ASSERT_THAT(tf.shape(), ElementsAre(0, 0));
        ASSERT_THAT(tf, ElementsAre());

        Tensor<float, 2> tf_m2(tf_m);
        tf_m2 = std::move(tf_m);
        ASSERT_THAT(tf_m2.shape(), ElementsAre(3, 2));
        ASSERT_THAT(tf_m2, ElementsAre(1, 1, 1, 2, 2, 2));
        ASSERT_THAT(tf_m.shape(), ElementsAre(0, 0));
        ASSERT_THAT(tf_m, ElementsAre());
    }
}

TEST(TensorTest, TestReshape) {
    Tensor<float, 2> t2f({3, 2}, {1, 1, 1, 2, 2, 2});
    t2f.resize({1, 4});
    EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 2));
    ASSERT_THAT(t2f.shape(), ElementsAre(1, 4));
    ASSERT_EQ(t2f.size(), 4);

    t2f.resize({2, 3}, 6);
    EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 2, 6, 6));
}

TEST(TestTensor, TestAverage) {
    Tensor<float, 2> t2({2, 3}, {1, 1, 1, 2, 2, 2});
    auto ret2 = t2.average();
    EXPECT_THAT(ret2, ElementsAre(1.5, 1.5, 1.5));

    Tensor<uint16_t, 3> t3({4, 3, 2});
    std::fill(t3.begin(), t3.begin() + 12, 1);
    std::fill(t3.begin() + 12, t3.end(), 2);
    auto ret3 = t3.average<float>();
    EXPECT_THAT(ret3, ElementsAre(1.5, 1.5, 1.5, 1.5, 1.5, 1.5));
}

TEST(TestImageGroup, TestConstructor) {
    {
        RawImageGroup g({1, 3, 2});
        ASSERT_TRUE(g.empty());
        ASSERT_FALSE(g.full());
    }
    {
        RawImageGroup g({1, 3, 2}, {1, 1, 1, 1, 1, 1});
        EXPECT_THAT(g, ElementsAre(1, 1, 1, 1, 1, 1));
        ASSERT_TRUE(g.full());

        RawImageGroup g_m(std::move(g));
        EXPECT_THAT(g_m, ElementsAre(1, 1, 1, 1, 1, 1));
        ASSERT_TRUE(g_m.full());
        EXPECT_THAT(g, ElementsAre());
        ASSERT_TRUE(g.empty());

        RawImageGroup g_m2(g_m);
        g_m2 = std::move(g_m);
        EXPECT_THAT(g_m2, ElementsAre(1, 1, 1, 1, 1, 1));
        ASSERT_TRUE(g_m2.full());
        EXPECT_THAT(g_m, ElementsAre());
        ASSERT_TRUE(g_m.empty()); 
    }
}

TEST(TestImageGroup, TestPushToRaw) {
    RawImageGroup g1({4, 3, 2});
    using ValueType = typename RawImageGroup::ValueType;

    {
        ValueType x[6] = {0, 0, 0, 0, 0, 0};
        g1.push(reinterpret_cast<char*>(x));
        ASSERT_FALSE(g1.empty());
        ASSERT_FALSE(g1.full());
    }

    for (size_t i = 1; i < g1.shape()[0]; ++i) {
        auto v = static_cast<ValueType>(i);
        ValueType x[6] = {v, v, v, v, v, v};
        g1.push(reinterpret_cast<char*>(x));
    }
    ASSERT_TRUE(g1.full());
    EXPECT_THAT(g1, ElementsAre(0, 0, 0, 0, 0, 0, 
                                1, 1, 1, 1, 1, 1, 
                                2, 2, 2, 2, 2, 2, 
                                3, 3, 3, 3, 3, 3));

    // test overwritten
    {
        ValueType x[6] = {4, 4, 4, 4, 4, 4};
        g1.push(reinterpret_cast<char*>(x));
        ASSERT_TRUE(g1.full());
        EXPECT_THAT(g1, ElementsAre(4, 4, 4, 4, 4, 4, 
                                    1, 1, 1, 1, 1, 1, 
                                    2, 2, 2, 2, 2, 2, 
                                    3, 3, 3, 3, 3, 3));
    }

    // test reset
    g1.reset();
    ASSERT_TRUE(g1.empty());
    ASSERT_FALSE(g1.full());
}

TEST(TestImageGroup, TestPushToRawWithDowmsampling) {
    RawImageGroup g1({4, 2, 3});
    using ValueType = typename RawImageGroup::ValueType;

    ValueType x1[28] = {
        1, 2, 3, 4, 5, 6, 7,
        2, 3, 4, 5, 6, 7, 8,
        3, 4, 5, 6, 7, 8, 9,
        4, 5, 6, 7, 8, 9, 10
    };
    g1.push(reinterpret_cast<char*>(x1), {4, 7});

    ValueType x2[24] = {
        1, 2, 3, 4, 5, 6,
        2, 3, 4, 5, 6, 7,
        3, 4, 5, 6, 7, 8,
        4, 5, 6, 7, 8, 9
    };
    g1.push(reinterpret_cast<char*>(x2), {4, 6});
    
    ValueType x3[12] = {
        1, 2, 3, 4, 5, 6,
        2, 3, 4, 5, 6, 7
    };
    g1.push(reinterpret_cast<char*>(x3), {2, 6});
    
    ValueType x4[12] = {
        1, 2, 3,
        2, 3, 4,
        3, 4, 5,
        4, 5, 6
    };
    g1.push(reinterpret_cast<char*>(x4), {4, 3});
    
    EXPECT_THAT(g1, ElementsAre(1, 3, 5, 3, 5, 7, 
                                1, 3, 5, 3, 5, 7, 
                                1, 3, 5, 2, 4, 6, 
                                1, 2, 3, 3, 4, 5));
}

TEST(TestImageGroup, TestPushToPro) {
    ProImageGroup g1({2, 3, 2});
    ProImageGroup::ValueType x[6] = {1, 2, 3, 4, 5, 6};
    g1.push(reinterpret_cast<char*>(x));
    EXPECT_THAT(g1, ElementsAre(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
}

}