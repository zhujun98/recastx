#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slicerecon/reconstructor.hpp"
#include "slicerecon/utils.hpp"


namespace slicerecon::test {

using ::testing::ElementsAreArray;
using ::testing::Pointwise;
using ::testing::FloatNear;

class ReconTest : public testing::Test {
  protected:
    int rows_ = 200;
    int cols_ = 120;
    int pixels_ = rows_ * cols_;

    int num_darks_ = 4;
    int num_flats_ = 6;
    int num_projections_ = 32;
    std::vector<float> angles_;
    int group_size_ = num_projections_;
    int slice_size_ = cols_;
    int preview_size_ = slice_size_ / 2;
    slicerecon::ReconstructMode recon_mode_ = slicerecon::ReconstructMode::alternating;

    std::string filter_name_ = "shepp";
    bool gaussian_pass_ = false;
    int threads_ = 4;

    std::array<float, 3> volume_min_point_ {0.0f, 0.0f, 0.0f};
    std::array<float, 3> volume_max_point_ {1.0f, 1.0f, 1.0f};
    std::array<float, 2> detector_size_ {1.0f, 1.0f};

    slicerecon::Reconstructor recon_ {rows_, cols_, threads_};

    void SetUp() override {
        angles_ = slicerecon::utils::defaultAngles(num_projections_);
    }

    void buildRecon() {
        recon_.initialize(num_darks_, num_flats_, num_projections_, 
                          group_size_, preview_size_, recon_mode_);
        recon_.initFilter(filter_name_, gaussian_pass_);
        recon_.setSolver(std::make_unique<slicerecon::ParallelBeamSolver>(
            rows_, cols_, num_projections_, angles_, 
            volume_min_point_, volume_max_point_, preview_size_, slice_size_, 
            false, detector_size_, recon_mode_
        ));
    }

    void pushData(ProjectionType pt, int start, int end, int a=1, int b=0) {
        std::vector<RawDtype> img(pixels_);
        for (int i = start; i < end; ++i) {
            std::fill(img.begin(), img.end(), a * i + b);
            recon_.pushProjection(pt, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
        }
    } 
};

TEST_F(ReconTest, TestPushProjectionException) {
    buildRecon();
    std::vector<RawDtype> img(pixels_);
    EXPECT_THROW(recon_.pushProjection(
        ProjectionType::dark, 0, {10, 10}, reinterpret_cast<char*>(img.data())), 
        std::runtime_error);
}

TEST_F(ReconTest, TestPushProjection) {
    // num_projections == group size
    buildRecon();

    // push darks
    pushData(ProjectionType::dark, 0, num_darks_);
    EXPECT_EQ(recon_.darks()[pixels_], 1);
    EXPECT_EQ(recon_.darks().back(), num_darks_ - 1);

    // push flats
    pushData(ProjectionType::flat, 0, num_flats_, 1, 10);
    EXPECT_EQ(recon_.flats()[0], 10);
    EXPECT_EQ(recon_.flats()[3 * pixels_ - 10], 12);

    // push projections (don't completely fill the buffer)
    pushData(ProjectionType::projection, 0, num_projections_ - 1, 10, 10);
    EXPECT_EQ(recon_.buffer()[0], 10.f);
    EXPECT_EQ(recon_.buffer()[(num_projections_ - 1) * pixels_ - 1], 10.f * (num_projections_ - 1)); 

    // push projections to fill the buffer
    pushData(ProjectionType::projection, num_projections_ - 1, num_projections_, 10, 10);
    EXPECT_THAT(std::vector<float>(recon_.buffer().begin(), recon_.buffer().begin() + 3), 
                Pointwise(FloatNear(1e-6), {0.257829f, 0.257829f, 0.257829f}));
    EXPECT_THAT(std::vector<float>(recon_.buffer().rbegin(), recon_.buffer().rbegin() + 3), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.365727f, -3.365727f}));
}

TEST_F(ReconTest, TestPushProjection2) {
    // number of projections > group size
    // the batch size for projection processing is now the group size
    group_size_ = 12;
    buildRecon();

    pushData(ProjectionType::dark, 0, num_darks_);
    pushData(ProjectionType::flat, 0, num_flats_, 1, 10);

    auto& buffer = recon_.buffer();
    auto& sino_buffer = recon_.sinoBuffer();

    pushData(ProjectionType::projection, 0, group_size_, 10, 10);
    EXPECT_THAT(std::vector<float>(buffer.begin(), buffer.begin() + 2), 
                Pointwise(FloatNear(1e-6), {0.257829f, 0.257829f}));

    pushData(ProjectionType::projection, group_size_, num_projections_, 10, 10);
    EXPECT_THAT(std::vector<float>(buffer.rbegin(), buffer.rbegin() + 2), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.365727f}));
    EXPECT_THAT(std::vector<float>(buffer.begin() + cols_ - 1, buffer.begin() + cols_ + 1), 
                Pointwise(FloatNear(1e-6), {0.257829f, 0.257829f}));
    EXPECT_THAT(std::vector<float>(buffer.rbegin() + cols_ - 1, buffer.rbegin() + cols_ + 1), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.365727f}));
    EXPECT_THAT(std::vector<float>(buffer.rbegin(), buffer.rbegin() + 2), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.365727f}));

    EXPECT_THAT(std::vector<float>(sino_buffer.begin(), sino_buffer.begin() + 2), 
                Pointwise(FloatNear(1e-6), {0.257829f, 0.257829f}));
    EXPECT_THAT(std::vector<float>(sino_buffer.begin() + cols_ - 1, sino_buffer.begin() + cols_ + 1), 
                Pointwise(FloatNear(1e-6), {0.257829f, -0.519875f}));
    EXPECT_THAT(std::vector<float>(sino_buffer.rbegin() + cols_ -1, sino_buffer.rbegin() + cols_ + 1), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.333826f}));
    EXPECT_THAT(std::vector<float>(sino_buffer.rbegin(), sino_buffer.rbegin() + 2), 
                Pointwise(FloatNear(1e-6), {-3.365727f, -3.365727f}));
}
TEST(TestUtils, TestComputeReciprocal) {
    int n = 3, pixels = 12;
    std::vector<RawDtype> darks {
        4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8, 
        1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2, 
        2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0
    };
    std::vector<RawDtype> flats {
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