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

#include "recon/buffer.hpp"
#include "recon/preprocessing.hpp"

namespace recastx::recon::test {

using ::testing::ElementsAreArray;
using ::testing::Pointwise;
using ::testing::FloatNear;


TEST(TestPreprocessing, TestCopyToBuffer) {
    {
        ProImageData src ({4, 4}, {
            0, 1, 2, 3,
            1, 2, 3, 4,
            2, 3, 4, 5,
            3, 4, 5, 6
        });
        ProImageData dst ({2, 2});
        details::copyToBuffer(dst, src, {2, 2});
        EXPECT_THAT(dst, ElementsAreArray({
            0, 2,
            2, 4
        }));
    }
    {
        ProImageData src ({7, 5}, {
            0, 1, 2, 3, 4,
            1, 2, 3, 4, 5,
            2, 3, 4, 5, 6,
            3, 4, 5, 6, 7,
            4, 5, 6, 7, 8,
            5, 6, 7, 8, 9,
            6, 7, 8, 9, 10,
        });
        ProImageData dst ({3, 2});
        details::copyToBuffer(dst, src, {2, 2});
        EXPECT_THAT(dst, ElementsAreArray({
            0, 2, 
            2, 4,
            4, 6
        }));
    }
    {
        ProImageData src ({7, 5}, {
            0, 1, 2, 3, 4,
            1, 2, 3, 4, 5,
            2, 3, 4, 5, 6,
            3, 4, 5, 6, 7,
            4, 5, 6, 7, 8,
            5, 6, 7, 8, 9,
            6, 7, 8, 9, 10,
        });
        ProImageData dst ({4, 3});
        details::copyToBuffer(dst, src, {2, 2});
        EXPECT_THAT(dst, ElementsAreArray({
            0, 2, 0,
            2, 4, 0,
            4, 6, 0,
            0, 0, 0
        }));
    }
    {
        ProImageData src ({7, 5}, {
            0, 1, 2, 3, 4,
            1, 2, 3, 4, 5,
            2, 3, 4, 5, 6,
            3, 4, 5, 6, 7,
            4, 5, 6, 7, 8,
            5, 6, 7, 8, 9,
            6, 7, 8, 9, 10,
        });
        ProImageData dst ({5, 4});
        details::copyToBuffer(dst, src, {2, 2});
        EXPECT_THAT(dst, ElementsAreArray({
            0, 0, 0, 0,
            0, 0, 2, 0,
            0, 2, 4, 0,
            0, 4, 6, 0,
            0, 0, 0, 0
        }));
    }
}

TEST(TestPreprocessing, TestComputeReciprocal) {

    using ValueType = RawImageData::ValueType;
    {
        std::array<size_t, 2> shape {4, 3};
        std::vector<RawImageData> darks;
        darks.emplace_back(shape, std::vector<ValueType>{4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8});
        darks.emplace_back(shape, std::vector<ValueType>{1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2});
        darks.emplace_back(shape, std::vector<ValueType>{2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0});
        
        std::vector<RawImageData> flats;
        flats.emplace_back(shape, std::vector<ValueType>{1, 9, 5, 1, 7, 9, 0, 6, 7, 1, 5, 6});
        flats.emplace_back(shape, std::vector<ValueType>{2, 4, 8, 1, 3, 9, 5, 6, 1, 1, 1, 7});
        flats.emplace_back(shape, std::vector<ValueType>{9, 9, 4, 1, 6, 8, 6, 9, 2, 4, 9, 4});

        ProImageData dark_avg(shape);
        ProImageData reciprocal(shape);
        computeReciprocal(darks, flats, dark_avg, reciprocal, {1, 1});

        EXPECT_THAT(dark_avg, ElementsAreArray({
            2.33333333, 4.,        3.33333333,
            0.66666667, 5.,        6.66666667,
            5.,         5.,        2.66666667,
            6.,         4.,        3.33333333}));
        EXPECT_THAT(reciprocal, Pointwise(FloatNear(1e-6), {
            0.59999996,  0.29999998, 0.42857143,
            3.0000002,   2.9999986,  0.5,
            -0.75000006, 0.5,        1.5000004,
            -0.25,       1.,         0.42857143}));
    }
    {
        std::array<size_t, 2> shape {2, 3};
        std::vector<RawImageData> darks;

        std::vector<RawImageData> flats;
        flats.emplace_back(shape, std::vector<ValueType>(6, 2));

        ProImageData dark_avg(shape);
        ProImageData reciprocal(shape);
        computeReciprocal(darks, flats, dark_avg, reciprocal, {1, 1});
        EXPECT_THAT(dark_avg, ElementsAreArray({0., 0., 0., 0., 0., 0.}));
        EXPECT_THAT(reciprocal, ElementsAreArray({0.5, 0.5, 0.5, 0.5, 0.5, 0.5}));
    }
    {
        std::array<size_t, 2> shape {2, 3};
        std::vector<RawImageData> darks;
        darks.emplace_back(shape, std::vector<ValueType>(6, 2));

        std::vector<RawImageData> flats;

        ProImageData dark_avg(shape);
        ProImageData reciprocal(shape);
        computeReciprocal(darks, flats, dark_avg, reciprocal, {1, 1});
        EXPECT_THAT(dark_avg, ElementsAreArray({2., 2., 2., 2., 2., 2.}));
        EXPECT_THAT(reciprocal, ElementsAreArray({1., 1., 1., 1., 1., 1.}));
    }
}

TEST(TestPreprocessing, TestCopyToSinogram) {

    // (chunk_idx, rows, cols) -> (rows, chunk_idx, cols).
    Tensor<int, 3> src ({2, 3, 4}, std::vector<int>(24, 1));

    {
        Tensor<int, 3> dst ({3, 2, 4});
        copyToSinogram(dst.data(), src, 0, 2, 3, 4, 0);
        EXPECT_THAT(dst, ElementsAreArray({1, 1, 1, 1,
                                           0, 0, 0, 0,
                                           1, 1, 1, 1,
                                           0, 0, 0, 0,
                                           1, 1, 1, 1,
                                           0, 0, 0, 0}));
    }

    {
        Tensor<int, 3> dst ({3, 2, 4});
        copyToSinogram(dst.data(), src, 1, 2, 3, 4, 0);
        EXPECT_THAT(dst, ElementsAreArray({0, 0, 0, 0,
                                           1, 1, 1, 1,
                                           0, 0, 0, 0,
                                           1, 1, 1, 1,
                                           0, 0, 0, 0,
                                           1, 1, 1, 1}));
    }

    {
        Tensor<int, 3> dst ({3, 2, 4});
        copyToSinogram(dst.data(), src, 1, 2, 3, 4, 1);
        EXPECT_THAT(dst, ElementsAreArray({0, 0, 0, 0,
                                           1, 1, 1, 0,
                                           0, 0, 0, 0,
                                           1, 1, 1, 0,
                                           0, 0, 0, 0,
                                           1, 1, 1, 0}));
    }

    {
        Tensor<int, 3> dst ({3, 2, 4});
        copyToSinogram(dst.data(), src, 1, 2, 3, 4, -1);
        EXPECT_THAT(dst, ElementsAreArray({0, 0, 0, 0,
                                           0, 1, 1, 1,
                                           0, 0, 0, 0,
                                           0, 1, 1, 1,
                                           0, 0, 0, 0,
                                           0, 1, 1, 1}));
    }
}

} // namespace recastx::recon::test