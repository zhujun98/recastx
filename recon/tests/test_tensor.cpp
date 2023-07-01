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
        Tensor<float, 1> t1f({3});
        EXPECT_THAT(t1f.shape(), ElementsAre(3));
        EXPECT_EQ(t1f.size(), 3);
    }
    {
        Tensor<float, 2> t2f({3, 2});
        EXPECT_THAT(t2f.shape(), ElementsAre(3, 2));
        EXPECT_EQ(t2f.size(), 6);

        t2f = {1, 1, 1, 2, 2, 2};

        Tensor<float, 2> t2f_c(t2f);
        ASSERT_THAT(t2f_c.shape(), ElementsAre(3, 2));
        EXPECT_THAT(t2f_c, ElementsAre(1, 1, 1, 2, 2, 2));

        Tensor<float, 2> t2f_c2;
        t2f_c2 = t2f;
        ASSERT_THAT(t2f_c2.shape(), ElementsAre(3, 2));
        EXPECT_THAT(t2f_c2, ElementsAre(1, 1, 1, 2, 2, 2));

        Tensor<float, 2> t2f_m(std::move(t2f));
        ASSERT_THAT(t2f_m.shape(), ElementsAre(3, 2));
        EXPECT_THAT(t2f_m, ElementsAre(1, 1, 1, 2, 2, 2));
        ASSERT_THAT(t2f.shape(), ElementsAre(0, 0));
        EXPECT_THAT(t2f, ElementsAre());

        Tensor<float, 2> t2f_m2(t2f_m);
        t2f_m2 = std::move(t2f_m);
        ASSERT_THAT(t2f_m2.shape(), ElementsAre(3, 2));
        EXPECT_THAT(t2f_m2, ElementsAre(1, 1, 1, 2, 2, 2));
        ASSERT_THAT(t2f_m.shape(), ElementsAre(0, 0));
        EXPECT_THAT(t2f_m, ElementsAre());
    }
}

TEST(TestTensor, TestAssignment) {
    {
        Tensor<float, 1> t1f({3});
        t1f = {1, 2, 3};
        EXPECT_THAT(t1f, ElementsAre(1, 2, 3));
    }
    {
        Tensor<float, 2> t2f({3, 2});
        t2f = {1, 1, 1, 2, 2, 2};
        EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 2, 2, 2));
        EXPECT_THAT(t2f.shape(), ElementsAre(3, 2));

        // t2f = {1, 1, 1, 2, 2, 2, 2}; // segfault

        t2f = {1, 1, 1, 3, 3};
        EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 3, 3, 2));
    }
    {
        Tensor<float, 2> t2f;
        EXPECT_THAT(t2f.shape(), ElementsAre(0, 0));
        // t2f = {1, 2, 3}; // segfault because shape is (0, 0)
    }
}

TEST(TensorTest, TestReshape) {
    Tensor<float, 2> t2f({3, 2});
    t2f = {1, 1, 1, 2, 2, 2};
    t2f.resize({1, 4});
    EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 2));
    ASSERT_THAT(t2f.shape(), ElementsAre(1, 4));
    ASSERT_EQ(t2f.size(), 4);

    t2f.resize({2, 3}, 6);
    EXPECT_THAT(t2f, ElementsAre(1, 1, 1, 2, 6, 6));
}

TEST(TestTensor, TestAverage) {
    Tensor<float, 2> t2({2, 3});
    t2 = {1, 1, 1, 2, 2, 2};
    auto ret2 = t2.average();
    EXPECT_THAT(ret2, ElementsAre(1.5, 1.5, 1.5));

    Tensor<uint16_t, 3> t3({4, 3, 2});
    std::fill(t3.begin(), t3.begin() + 12, 1);
    std::fill(t3.begin() + 12, t3.end(), 2);
    auto ret3 = t3.average<float>();
    EXPECT_THAT(ret3, ElementsAre(1.5, 1.5, 1.5, 1.5, 1.5, 1.5));
}

TEST(TestImageGroup, TestConstructor) {
    RawImageGroup g1({1, 3, 2});
    ASSERT_TRUE(g1.empty());
    ASSERT_FALSE(g1.full());

    g1 = {1, 1, 1, 1, 1, 1};
    EXPECT_THAT(g1, ElementsAre(1, 1, 1, 1, 1, 1));
    ASSERT_TRUE(g1.full());

    RawImageGroup g1_m(std::move(g1));
    EXPECT_THAT(g1_m, ElementsAre(1, 1, 1, 1, 1, 1));
    ASSERT_TRUE(g1_m.full());
    EXPECT_THAT(g1, ElementsAre());
    ASSERT_TRUE(g1.empty());

    RawImageGroup g1_m2(g1_m);
    g1_m2 = std::move(g1_m);
    EXPECT_THAT(g1_m2, ElementsAre(1, 1, 1, 1, 1, 1));
    ASSERT_TRUE(g1_m2.full());
    EXPECT_THAT(g1_m, ElementsAre());
    ASSERT_TRUE(g1_m.empty());  
}

TEST(TestImageGroup, TestPushToRaw) {
    RawImageGroup g1({4, 3, 2});
    using value_type = typename RawImageGroup::value_type;

    {
        value_type x[6] = {0, 0, 0, 0, 0, 0};
        g1.push(reinterpret_cast<char*>(x));
        ASSERT_FALSE(g1.empty());
        ASSERT_FALSE(g1.full());
    }

    for (size_t i = 1; i < g1.shape()[0]; ++i) {
        auto v = static_cast<value_type>(i);
        value_type x[6] = {v, v, v, v, v, v};
        g1.push(reinterpret_cast<char*>(x));
    }
    ASSERT_TRUE(g1.full());
    EXPECT_THAT(g1, ElementsAre(0, 0, 0, 0, 0, 0, 
                                1, 1, 1, 1, 1, 1, 
                                2, 2, 2, 2, 2, 2, 
                                3, 3, 3, 3, 3, 3));

    // test overwritten
    {
        value_type x[6] = {4, 4, 4, 4, 4, 4};
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
    using value_type = typename RawImageGroup::value_type;

    value_type x1[28] = {
        1, 2, 3, 4, 5, 6, 7,
        2, 3, 4, 5, 6, 7, 8,
        3, 4, 5, 6, 7, 8, 9,
        4, 5, 6, 7, 8, 9, 10
    };
    g1.push(reinterpret_cast<char*>(x1), {4, 7});

    value_type x2[24] = {
        1, 2, 3, 4, 5, 6,
        2, 3, 4, 5, 6, 7,
        3, 4, 5, 6, 7, 8,
        4, 5, 6, 7, 8, 9
    };
    g1.push(reinterpret_cast<char*>(x2), {4, 6});
    
    value_type x3[12] = {
        1, 2, 3, 4, 5, 6,
        2, 3, 4, 5, 6, 7
    };
    g1.push(reinterpret_cast<char*>(x3), {2, 6});
    
    value_type x4[12] = {
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
    ProImageGroup::value_type x[6] = {1, 2, 3, 4, 5, 6};
    g1.push(reinterpret_cast<char*>(x));
    EXPECT_THAT(g1, ElementsAre(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
}

}