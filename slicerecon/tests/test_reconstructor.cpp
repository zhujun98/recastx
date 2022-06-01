#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slicerecon/slicerecon.hpp"
#include "slicerecon/data_types.hpp"
#include "slicerecon/reconstruction/utils.hpp"


namespace slicerecon::test {

using ::testing::ElementsAreArray;
using ::testing::Pointwise;
using ::testing::FloatNear;

class ReconTest : public testing::Test {
  protected:
    int rows_ = 300;
    int cols_ = 200;
    int pixels_ = rows_ * cols_;

    int darks_ = 4;
    int flats_ = 6;
    int projections_ = 32;

    slicerecon::PaganinSettings paganin_ {};
    slicerecon::Settings params_ {
        32, 32, 32, 8, 
        darks_, flats_, projections_, 
        slicerecon::ReconstructMode::alternating,
        false, false, false, paganin_, "shepp-logan"
    };
    slicerecon::Geometry geom_ {
        rows_, cols_, 128,
        {}, true, false,
        {0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        0.0f };
    slicerecon::Reconstructor recon_ {params_, geom_};

  public:
    slicerecon::Settings params() const { return params_; }
    slicerecon::Geometry geom() const { return geom_; }
};

TEST_F(ReconTest, TestPushProjectionException) {
    std::vector<raw_dtype> img(pixels_);
    EXPECT_THROW(recon_.pushProjection(
        ProjectionType::dark, 0, {10, 10}, reinterpret_cast<char*>(img.data())), 
        std::runtime_error);
}
 
TEST_F(ReconTest, TestPushProjection) {
    std::vector<raw_dtype> img(pixels_);

    // push darks
    for (int i = 0; i < darks_; ++i) {
        std::fill(img.begin(), img.end(), i);
        recon_.pushProjection(
            ProjectionType::dark, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
    }
    EXPECT_EQ(recon_.darks()[pixels_], 1);
    EXPECT_EQ(recon_.darks().back(), darks_ - 1);

    // push flats
    for (int i = 0; i < flats_; ++i) {
        std::fill(img.begin(), img.end(), i + 10);
        recon_.pushProjection(
            ProjectionType::flat, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
    }
    EXPECT_EQ(recon_.flats()[0], 10);
    EXPECT_EQ(recon_.flats()[3*pixels_ - 10], 12);

    // push projections (don't completely fill the buffer)
    for (int i = 0; i < projections_ - 1; ++i) {
        std::fill(img.begin(), img.end(), 10 * i + 10);
        recon_.pushProjection(
            ProjectionType::standard, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
    }
    EXPECT_EQ(recon_.buffer()[0], 10.f);
    EXPECT_EQ(recon_.buffer()[(projections_ - 1) * pixels_ - 1], 10.f * (projections_ - 1)); 
}

TEST(TestUtils, TestComputeReciprocal) {
    int n = 3, pixels = 12;
    std::vector<raw_dtype> darks {
        4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8, 
        1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2, 
        2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0
    };
    std::vector<raw_dtype> flats {
        1, 9, 5, 1, 7, 9, 0, 6, 7, 1, 5, 6, 
        2, 4, 8, 1, 3, 9, 5, 6, 1, 1, 1, 7, 
        9, 9, 4, 1, 6, 8, 6, 9, 2, 4, 9, 4
    };
    std::vector<float> reciprocal(pixels);
    std::vector<float> dark_avg(pixels);

    slicerecon::utils::computeReciprocal(darks, flats, pixels, reciprocal, dark_avg);

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

} // slicerecon::test