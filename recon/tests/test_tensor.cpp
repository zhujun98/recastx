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
    ASSERT_THAT(t2f.shape(), ElementsAre(2, 3));
}

TEST(TestTensor, TestScalarArithmetic) {
    {
        using TensorType = Tensor<uint16_t, 2>;
        auto t = TensorType({2, 2}, {2, 2, 3, 3});
        {
            TensorType ret = t + 1.f;
            EXPECT_THAT(ret, ElementsAre(3, 3, 4, 4));
            ret += 1.f;
            EXPECT_THAT(ret, ElementsAre(4, 4, 5, 5));
        }
        {
            TensorType ret = t - 1.f;
            EXPECT_THAT(ret, ElementsAre(1, 1, 2, 2));
            ret -= 1.f;
            EXPECT_THAT(ret, ElementsAre(0, 0, 1, 1));

        }
        {
            TensorType ret = t / 2.f;
            EXPECT_THAT(ret, ElementsAre(1, 1, 1, 1));
            ret /= 2.f;
            EXPECT_THAT(ret, ElementsAre(0, 0, 0, 0));
        }
        {
            TensorType ret = t * 2.f;
            EXPECT_THAT(ret, ElementsAre(4, 4, 6, 6));
            ret *= 2.f;
            EXPECT_THAT(ret, ElementsAre(8, 8, 12, 12));
        }
    }
    {
        using TensorType = Tensor<float, 2>;
        auto t = TensorType({2, 2}, {2.f, 2.f, 3.f, 3.f});
        {
            TensorType ret = t + 1;
            EXPECT_THAT(ret, ElementsAre(3.f, 3.f, 4.f, 4.f));
            ret += 1;
            EXPECT_THAT(ret, ElementsAre(4.f, 4.f, 5.f, 5.f));
        }
        {
            TensorType ret = t - 1;
            EXPECT_THAT(ret, ElementsAre(1.f, 1.f, 2.f, 2.f));
            ret -= 1;
            EXPECT_THAT(ret, ElementsAre(0.f, 0.f, 1.f, 1.f));
        }
        {
            TensorType ret = t / 2;
            EXPECT_THAT(ret, ElementsAre(1.f, 1.f, 1.5f, 1.5f));
            ret /= 2;
            EXPECT_THAT(ret, ElementsAre(0.5f, 0.5f, 0.75f, 0.75f));
        }
        {
            TensorType ret = t * 2;
            EXPECT_THAT(ret, ElementsAre(4.f, 4.f, 6.f, 6.f));
            ret *= 2;
            EXPECT_THAT(ret, ElementsAre(8.f, 8.f, 12.f, 12.f));
        }
    }
}

TEST(TestTensor, TestVectorArithmetic) {
    {
        using TensorType = Tensor<int, 2>;
        auto t1 = TensorType({2, 2}, {1, 2, 3, 4});
        auto t2 = TensorType({2, 2}, {4, 3, 2, 1});
        auto t3 = TensorType({2, 2}, {1, -1, 1, -1});

        {
            TensorType ret = t1 + t2 + t3;
            EXPECT_THAT(ret, ElementsAre(6, 4, 6, 4));
            ret += t1;
            EXPECT_THAT(ret, ElementsAre(7, 6, 9, 8));
        }
        {
            TensorType ret = t1 - t2 - t3;
            EXPECT_THAT(ret, ElementsAre(-4, 0, 0, 4));
            ret -= t1;
            EXPECT_THAT(ret, ElementsAre(-5, -2, -3, 0));
        }
    }
    {
        using TensorType = Tensor<float, 2>;
        auto t1 = TensorType({2, 2}, {1.f, 2.f, 3.f, 4.f});
        auto t2 = TensorType({2, 2}, {4.f, 3.f, 2.f, 1.f});
        auto t3 = TensorType({2, 2}, {1.f, -1.f, 1.f, -1.f});

        {
            TensorType ret = t1 + t2 + t3;
            EXPECT_THAT(ret, ElementsAre(6.f, 4.f, 6.f, 4.f));
            ret += t1;
            EXPECT_THAT(ret, ElementsAre(7.f, 6.f, 9.f, 8.f));
        }
        {
            TensorType ret = t1 - t2 - t3;
            EXPECT_THAT(ret, ElementsAre(-4.f, 0.f, 0.f, 4.f));
            ret -= t1;
            EXPECT_THAT(ret, ElementsAre(-5.f, -2.f, -3.f, 0.f));
        }
    }
}

TEST(TestMath, TestAverage) {
    std::array<size_t, 2> s{2, 3};
    std::vector<Tensor<uint16_t, 2>> src;
    src.emplace_back(s, std::vector<uint16_t>(6, 1));
    src.emplace_back(s, std::vector<uint16_t>(6, 2));
    src.emplace_back(s, std::vector<uint16_t>(6, 65535));

    {
        Tensor<float, 2> dst = math::average<float>(src);
        EXPECT_THAT(dst, ElementsAre(21846.f, 21846.f, 21846.f, 21846.f, 21846.f, 21846.f));
    }
    {
        Tensor<float, 2> dst(s);
        math::average(src, dst);
        EXPECT_THAT(dst, ElementsAre(21846.f, 21846.f, 21846.f, 21846.f, 21846.f, 21846.f));
    }
    {
        Tensor<double, 2> dst = math::average(src);
        EXPECT_THAT(dst, ElementsAre(21846., 21846., 21846., 21846., 21846., 21846.));
    }
    {
        Tensor<uint16_t, 2> dst = math::average<uint16_t>(src);
        EXPECT_THAT(dst, ElementsAre(0, 0, 0, 0, 0, 0));
    }
}

} // namespace recastx::recon::test
