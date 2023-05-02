#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/preprocessing.hpp"


namespace recastx::recon::test {

using ::testing::ElementsAreArray;
using ::testing::Pointwise;
using ::testing::FloatNear;


TEST(TestPreprocessing, TestDownsample) {
    {
        ProImageData src ({4, 4});
        src = {
            0, 1, 2, 3,
            1, 2, 3, 4,
            2, 3, 4, 5,
            3, 4, 5, 6
        };
        ProImageData dst ({2, 2});
        downsample(src, dst);
        EXPECT_THAT(dst, ElementsAreArray({
            0, 2,
            2, 4
        }));
    }
    {
        ProImageData src ({7, 5});
        src = {
            0, 1, 2, 3, 4,
            1, 2, 3, 4, 5,
            2, 3, 4, 5, 6,
            3, 4, 5, 6, 7,
            4, 5, 6, 7, 8,
            5, 6, 7, 8, 9,
            6, 7, 8, 9, 10,
        };
        ProImageData dst ({3, 2});
        downsample(src, dst);
        EXPECT_THAT(dst, ElementsAreArray({
            0, 2, 
            2, 4,
            4, 6
        }));
    }
}

TEST(TestPreprocessing, TestComputeReciprocal) {
    std::array<size_t, 2> shape {4, 3};
    RawImageGroup darks({3, shape[0], shape[1]});
    darks = {
        4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8, 
        1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2, 
        2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0
    };
    RawImageGroup flats({3, shape[0], shape[1]});
    flats = {
        1, 9, 5, 1, 7, 9, 0, 6, 7, 1, 5, 6, 
        2, 4, 8, 1, 3, 9, 5, 6, 1, 1, 1, 7, 
        9, 9, 4, 1, 6, 8, 6, 9, 2, 4, 9, 4
    };

    auto [dark_avg, reciprocal] = computeReciprocal(darks, flats);

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

} // namespace recastx::recon::test