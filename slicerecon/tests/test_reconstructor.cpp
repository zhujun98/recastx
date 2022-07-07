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
    int rows_ = 4;
    int cols_ = 5;
    int pixels_ = rows_ * cols_;

    int num_darks_ = 4;
    int num_flats_ = 6;
    int num_projections_ = 16;
    std::vector<float> angles_;
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
                          preview_size_, recon_mode_);
        recon_.initFilter(filter_name_, gaussian_pass_);
        recon_.setSolver(std::make_unique<slicerecon::ParallelBeamSolver>(
            rows_, cols_, angles_, 
            volume_min_point_, volume_max_point_, preview_size_, slice_size_, 
            false, detector_size_, recon_mode_
        ));
    }

    void pushDarks(int n) {
        std::vector<RawDtype> img(pixels_, 0);
        for (int i = 0; i < n; ++i) {
            recon_.pushProjection(
                ProjectionType::dark, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
        }
    } 

    void pushFlats(int n) {
        std::vector<RawDtype> img(pixels_, 1);
        for (int i = 0; i < n; ++i) {
            recon_.pushProjection(
                ProjectionType::flat, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
        }
    } 

    void pushProjection(int start, int end) {
        std::vector<RawDtype> img_base {
            2, 5, 3, 7, 1,
            4, 6, 2, 9, 5,
            1, 3, 7, 5, 8,
            6, 8, 8, 7, 3
        };
        for (int i = start; i < end; ++i) {
            std::vector<RawDtype> img(img_base);
            if (i % 2 == 1) {
                for (size_t i = 0; i < img.size(); ++i) img[i] += 1;
            }
            recon_.pushProjection(
                ProjectionType::projection, i, {rows_, cols_}, reinterpret_cast<char*>(img.data()));
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

    pushDarks(num_darks_);
    pushFlats(num_flats_);

    // buffer will be swapped after the processing
    auto& buffer1 = recon_.buffer().front();
    auto& buffer2 = recon_.buffer().back();
    auto& sino_buffer = recon_.sinoBuffer();

    // push projections (don't completely fill the buffer)
    pushProjection(0, num_projections_ - 1);
    EXPECT_EQ(buffer1[0], 2.f);
    EXPECT_EQ(buffer1[(num_projections_ - 1) * pixels_ - 1], 3.f); 

    // push projections to fill the buffer
    pushProjection(num_projections_ - 1, num_projections_);
    EXPECT_THAT(std::vector<float>(buffer2.begin(), buffer2.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.099537f, -0.214807f, 0.464008f, -0.369369f, 0.020631f}));
    EXPECT_THAT(std::vector<float>(buffer2.end() - 10, buffer2.end()), 
                Pointwise(FloatNear(1e-6), { 0.443812f,  0.056262f, -0.205481f,  0.034181f, -0.328773f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f,  0.262528f}));
    EXPECT_THAT(std::vector<float>(sino_buffer.begin(), sino_buffer.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.101732f, -0.201946f, 0.119072f, -0.369920f, 0.351062f}));
    EXPECT_THAT(std::vector<float>(sino_buffer.end() - 10, sino_buffer.end()), 
                Pointwise(FloatNear(1e-6), {-0.040253f, -0.094602f, -0.078659f, -0.107789f, 0.3213040f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f, 0.262528f}));
}

TEST(TestUtils, TestComputeReciprocal) {
    int pixels = 12;
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