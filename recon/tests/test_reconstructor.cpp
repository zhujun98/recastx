#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/server.hpp"
#include "recon/utils.hpp"


namespace tomcat::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

class ReconTest : public testing::Test {
  protected:
    int num_cols_ = 5;
    int num_rows_ = 4;
    int pixels_ = num_rows_ * num_cols_;

    int num_darks_ = 4;
    int num_flats_ = 6;
    int buffer_size_ = 100;
    int num_angles_ = 16;
    std::vector<float> angles_;
    int slice_size_ = num_cols_;
    int preview_size_ = slice_size_ / 2;

    std::string filter_name_ = "shepp";
    bool gaussian_pass_ = false;
    int threads_ = 4;

    std::array<float, 3> volume_min_point_ {0.0f, 0.0f, 0.0f};
    std::array<float, 3> volume_max_point_ {1.0f, 1.0f, 1.0f};
    std::array<float, 2> detector_size_ {1.0f, 1.0f};

    Server server_ {threads_};

    void SetUp() override {
        angles_ = utils::defaultAngles(num_angles_);
        buildRecon();
        server_.startPreprocessing();
    }

    void buildRecon() {
        server_.init(num_cols_, num_rows_, num_angles_, num_darks_, num_flats_, slice_size_, preview_size_, buffer_size_);
        server_.initFilter(filter_name_, num_rows_, num_cols_, gaussian_pass_);

        server_.setReconstructor(tomcat::recon::createReconstructor(
            false, num_rows_, num_cols_, num_angles_, 1.f, 1.f, 0.0f, 0.0f, 
            slice_size_, preview_size_, volume_min_point_, volume_max_point_));
    }

    void pushDarks(int n) {
        std::vector<RawDtype> img(pixels_, 0);
        for (int i = 0; i < n; ++i) {
            server_.pushProjection(
                ProjectionType::dark, i, {num_rows_, num_cols_}, reinterpret_cast<char*>(img.data()));
        }
    } 

    void pushFlats(int n) {
        std::vector<RawDtype> img(pixels_, 1);
        for (int i = 0; i < n; ++i) {
            server_.pushProjection(
                ProjectionType::flat, i, {num_rows_, num_cols_}, reinterpret_cast<char*>(img.data()));
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
            server_.pushProjection(
                ProjectionType::projection, i, {num_rows_, num_cols_}, reinterpret_cast<char*>(img.data()));
        }
    }
};

TEST_F(ReconTest, TestPushProjectionException) {
    std::vector<RawDtype> img(pixels_);
    EXPECT_THROW(server_.pushProjection(
        ProjectionType::dark, 0, {10, 10}, reinterpret_cast<char*>(img.data())), 
        std::runtime_error);
}

TEST_F(ReconTest, TestPushProjection) {
    pushDarks(num_darks_);
    pushFlats(num_flats_);

    auto& sino = server_.sinoBuffer().ready();

    // push projections (don't completely fill the buffer)
    pushProjection(0, num_angles_ - 1);
    auto& projs_ready = server_.buffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[(num_angles_ - 1) * pixels_ - 1], 3.f); 

    // push projections to fill the buffer
    pushProjection(num_angles_ - 1, num_angles_);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto& projs_front = server_.buffer().front();
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
    pushProjection(num_angles_, num_angles_ + 1);
    EXPECT_EQ(server_.buffer().occupied(), 2);

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    // buffer should have been reset
    pushProjection(0, 1);
    EXPECT_EQ(server_.buffer().occupied(), 1);
}

TEST_F(ReconTest, TestPushProjectionUnordered) {
    pushDarks(num_darks_);
    pushFlats(num_flats_);

    pushProjection(0, num_angles_ - 3);
    int overflow = 3;
    pushProjection(num_angles_ - 1, num_angles_ + overflow);
    auto& projs_ready = server_.buffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[overflow * pixels_ - 1], 3.f);
    EXPECT_EQ(projs_ready[num_angles_ * pixels_ - 1], 4.f);

    pushProjection(num_angles_ - 3, num_angles_ - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto& projs_front = server_.buffer().front();
    // FIXME: unittest fails from now and then
    EXPECT_THAT(std::vector<float>(projs_front.begin(), projs_front.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.099537f, -0.214807f, 0.464008f, -0.369369f, 0.020631f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()), 
                Pointwise(FloatNear(1e-6), { 0.443812f,  0.056262f, -0.205481f,  0.034181f, -0.328773f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f,  0.262528f}));

    auto& sino = server_.sinoBuffer().ready();
    EXPECT_THAT(std::vector<float>(sino.begin(), sino.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.101732f, -0.201946f, 0.119072f, -0.369920f, 0.351062f}));
    EXPECT_THAT(std::vector<float>(sino.end() - 10, sino.end()), 
                Pointwise(FloatNear(1e-6), {-0.040253f, -0.094602f, -0.078659f, -0.107789f, 0.3213040f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f, 0.262528f}));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pushProjection(num_angles_ + overflow, 2 * num_angles_ - 1);
    // trigger warn log message, there must be at least one unfilled group in the buffer
    pushProjection(0, 1);
}

TEST_F(ReconTest, TestUploading) {
    server_.startUploading();
}

TEST_F(ReconTest, TestReconstructing) {
    server_.startUploading();
    server_.startReconstructing();
}

} // tomcat::recon::test