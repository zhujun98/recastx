#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>

#include "recon/tensor.hpp"

namespace tomcat::recon::test {

using ::testing::ElementsAre;

TEST(TestTensor, TestGeneral) {
    Tensor<float, 2> t2({3, 2});
    std::fill(t2.begin(), t2.begin() + 3, 1);
    std::fill(t2.begin() + 3, t2.end(), 2);
    EXPECT_THAT(t2, ElementsAre(1, 1, 1, 2, 2, 2));
    ASSERT_THAT(t2.shape(), ElementsAre(3, 2));
    ASSERT_EQ(t2.size(), 6);

    t2.resize({1, 4});
    EXPECT_THAT(t2, ElementsAre(1, 1, 1, 2));
    ASSERT_THAT(t2.shape(), ElementsAre(1, 4));
    ASSERT_EQ(t2.size(), 4);

    t2.resize({2, 3}, 6);
    EXPECT_THAT(t2, ElementsAre(1, 1, 1, 2, 6, 6));
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

TEST(TestImageGroup, TestGeneral) {
    RawImageGroup g1({2, 3, 1});
    ASSERT_TRUE(g1.empty());
    ASSERT_FALSE(g1.full());

    g1 = {1, 1, 1, 1, 1, 1};
    EXPECT_THAT(g1, ElementsAre(1, 1, 1, 1, 1, 1));
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

    g1.reset();
    ASSERT_TRUE(g1.empty());
    ASSERT_FALSE(g1.full());
}

TEST(TestImageGroup, TestPushToPro) {
    ProImageGroup g1({2, 3, 2});
    ProImageGroup::value_type x[6] = {1, 2, 3, 4, 5, 6};
    g1.push(reinterpret_cast<char*>(x));
    EXPECT_THAT(g1, ElementsAre(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 
                                0.f, 0.f, 0.f, 0.f, 0.f, 0.f));
}

}