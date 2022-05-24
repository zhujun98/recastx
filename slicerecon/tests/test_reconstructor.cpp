#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include "slicerecon/slicerecon.hpp"

namespace slicerecon::test {

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

} // slicerecon::test