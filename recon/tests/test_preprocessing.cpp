#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/preprocessing.hpp"


namespace tomcat::recon::test {

using ::testing::ElementsAreArray;
using ::testing::Pointwise;
using ::testing::FloatNear;

TEST(TestUtils, TestComputeReciprocal) {
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
    ProImageData reciprocal(shape);
    ProImageData dark_avg(shape);

    computeReciprocal(darks, flats, reciprocal, dark_avg);

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

} // tomcat::recon::test