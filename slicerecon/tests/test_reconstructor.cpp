#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slicerecon/reconstructor.hpp"
#include "slicerecon/utils.hpp"


namespace tomop::slicerecon::test {

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
    int group_size_ = 16;
    int buffer_size_ = 100;
    std::vector<float> angles_;
    int slice_size_ = cols_;
    int preview_size_ = slice_size_ / 2;

    std::string filter_name_ = "shepp";
    bool gaussian_pass_ = false;
    int threads_ = 4;

    std::array<float, 3> volume_min_point_ {0.0f, 0.0f, 0.0f};
    std::array<float, 3> volume_max_point_ {1.0f, 1.0f, 1.0f};
    std::array<float, 2> detector_size_ {1.0f, 1.0f};

    slicerecon::Reconstructor recon_ {rows_, cols_, threads_};

    void SetUp() override {
        angles_ = slicerecon::utils::defaultAngles(group_size_);
        buildRecon();
        recon_.startProcessing();
    }

    void buildRecon() {
        recon_.initialize(num_darks_, num_flats_, group_size_, buffer_size_, 
                          preview_size_, slice_size_);
        recon_.initFilter(filter_name_, gaussian_pass_);
        recon_.setSolver(std::make_unique<slicerecon::ParallelBeamSolver>(
            rows_, cols_, angles_, 
            volume_min_point_, volume_max_point_, preview_size_, slice_size_, 
            false, detector_size_
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
    std::vector<RawDtype> img(pixels_);
    EXPECT_THROW(recon_.pushProjection(
        ProjectionType::dark, 0, {10, 10}, reinterpret_cast<char*>(img.data())), 
        std::runtime_error);
}

TEST_F(ReconTest, TestPushProjection) {
    pushDarks(num_darks_);
    pushFlats(num_flats_);

    auto& sino = recon_.sinoBuffer().ready();

    // push projections (don't completely fill the buffer)
    pushProjection(0, group_size_ - 1);
    auto& projs_ready = recon_.buffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[(group_size_ - 1) * pixels_ - 1], 3.f); 

    // push projections to fill the buffer
    pushProjection(group_size_ - 1, group_size_);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto& projs_front = recon_.buffer().front();
    EXPECT_THAT(std::vector<float>(projs_front.begin(), projs_front.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.099537f, -0.214807f, 0.464008f, -0.369369f, 0.020631f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()), 
                Pointwise(FloatNear(1e-6), { 0.443812f,  0.056262f, -0.205481f,  0.034181f, -0.328773f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f,  0.262528f}));
    EXPECT_THAT(std::vector<float>(sino.begin(), sino.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.101732f, -0.201946f, 0.119072f, -0.369920f, 0.351062f}));
    EXPECT_THAT(std::vector<float>(sino.end() - 10, sino.end()), 
                Pointwise(FloatNear(1e-6), {-0.040253f, -0.094602f, -0.078659f, -0.107789f, 0.3213040f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f, 0.262528f}));
}

TEST_F(ReconTest, TestMemoryBufferReset) {
    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, 1);
    pushProjection(group_size_, group_size_ + 1);
    EXPECT_EQ(recon_.buffer().occupied(), 2);

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    // buffer should have been reset
    pushProjection(0, 1);
    EXPECT_EQ(recon_.buffer().occupied(), 1);
}

TEST_F(ReconTest, TestPushProjectionUnordered) {
    pushDarks(num_darks_);
    pushFlats(num_flats_);

    pushProjection(0, group_size_ - 3);
    int overflow = 3;
    pushProjection(group_size_ - 1, group_size_ + overflow);
    auto& projs_ready = recon_.buffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[overflow * pixels_ - 1], 3.f);
    EXPECT_EQ(projs_ready[group_size_ * pixels_ - 1], 4.f);

    pushProjection(group_size_ - 3, group_size_ - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto& projs_front = recon_.buffer().front();
    // FIXME: unittest fails from now and then
    EXPECT_THAT(std::vector<float>(projs_front.begin(), projs_front.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.099537f, -0.214807f, 0.464008f, -0.369369f, 0.020631f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()), 
                Pointwise(FloatNear(1e-6), { 0.443812f,  0.056262f, -0.205481f,  0.034181f, -0.328773f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f,  0.262528f}));

    auto& sino = recon_.sinoBuffer().ready();
    EXPECT_THAT(std::vector<float>(sino.begin(), sino.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.101732f, -0.201946f, 0.119072f, -0.369920f, 0.351062f}));
    EXPECT_THAT(std::vector<float>(sino.end() - 10, sino.end()), 
                Pointwise(FloatNear(1e-6), {-0.040253f, -0.094602f, -0.078659f, -0.107789f, 0.3213040f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f, 0.262528f}));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pushProjection(group_size_ + overflow, 2 * group_size_ - 1);
    // trigger warn log message, there must be at least one unfilled group in the buffer
    pushProjection(0, 1);
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

} // tomop::slicerecon::test