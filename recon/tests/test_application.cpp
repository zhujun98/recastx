/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/application.hpp"
#include "recon/preprocessing.hpp"


namespace recastx::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

class MockDaqClient : public DaqClientInterface {

  public:

    void start() {}

    void startAcquiring() {}
    void stopAcquiring() {}

    template<typename... Args>
    void push(Args&&... args) {
        queue_.emplace(std::forward<Args>(args)...);
    }
};


class ApplicationTest : public testing::Test {

  protected:

    size_t num_cols_ = 5;
    size_t num_rows_ = 4;
    size_t pixels_ = num_rows_ * num_cols_;
    float pixel_width_ = 1.0f;
    float pixel_height_ = 1.0f;
    float src2origin = 0.f;
    float origin2det = 0.f;

    uint32_t downsampling_col_ = 1;
    uint32_t downsampling_row_ = 1;

    size_t num_darks_ = 4;
    size_t num_flats_ = 6;
    size_t buffer_size_ = 100;
    size_t num_angles_ = 16;
    size_t slice_size_ = num_cols_;
    size_t preview_size_ = slice_size_ / 2;

    std::string filter_name_ = "shepp";
    bool gaussian_lowpass_filter_ = false;
    uint32_t threads_ = 2;

    std::unique_ptr<DaqClientInterface> daq_client_;

    const RpcServerConfig rpc_cfg {12347};
    const ImageprocParams imgproc_params {
        threads_, downsampling_col_, downsampling_row_,
        {filter_name_, gaussian_lowpass_filter_}    
    };

    Application app_;

    ApplicationTest() : 
            daq_client_(new MockDaqClient()),
            app_ {buffer_size_, imgproc_params, daq_client_.get(), rpc_cfg} {
        app_.setScanMode(ScanMode_Mode_DISCRETE, num_angles_);
    }

    ~ApplicationTest() override {
        app_.onStateChanged(ServerState_State::ServerState_State_READY);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };

    void SetUp() override { 
        app_.setFlatFieldCorrectionParams(num_darks_, num_flats_);
        app_.setProjectionGeometry(recastx::BeamShape::PARALELL, num_cols_, num_rows_,
                                   pixel_width_, pixel_height_, 
                                   src2origin, origin2det, num_angles_);
        app_.setReconGeometry(std::nullopt, std::nullopt, 
                              std::nullopt, std::nullopt, 
                              std::nullopt, std::nullopt,
                              std::nullopt, std::nullopt);
    }

    void pushDarks(int n) {
        std::vector<RawDtype> img(pixels_, 0);
        for (int i = 0; i < n; ++i) {
            dynamic_cast<MockDaqClient*>(daq_client_.get())->push(
                ProjectionType::DARK, i, num_cols_, num_rows_, 
                zmq::message_t(reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype)));
        }
    } 

    void pushFlats(int n) {
        std::vector<RawDtype> img(pixels_, 1);
        for (int i = 0; i < n; ++i) {
            dynamic_cast<MockDaqClient*>(daq_client_.get())->push(
                ProjectionType::FLAT, i, num_cols_, num_rows_, 
                zmq::message_t(reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype)));
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
            dynamic_cast<MockDaqClient*>(daq_client_.get())->push(
                ProjectionType::PROJECTION, i, num_cols_, num_rows_,
                zmq::message_t(reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype)));
       }
    }
};

TEST_F(ApplicationTest, TestPushProjection) {
    app_.startAcquiring();
    app_.startPreprocessing();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    // push projections (don't completely fill the buffer)
    pushProjection(0, num_angles_ - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto& sino = app_.sinoBuffer().ready();
    auto& projs_ready = app_.rawBuffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[(num_angles_ - 1) * pixels_ - 1], 3.f); 

    // push projections to fill the buffer
    pushProjection(num_angles_ - 1, num_angles_);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto& projs_front = app_.rawBuffer().front();
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

TEST_F(ApplicationTest, TestMemoryBufferReset) {
    app_.startAcquiring();
    app_.startPreprocessing();
    app_.onStateChanged(ServerState_State::ServerState_State_ACQUIRING);

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, 1);
    pushProjection(num_angles_, num_angles_ + 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(app_.rawBuffer().occupied(), 2);

    // test reset when darks or flats received after projections
    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(app_.rawBuffer().occupied(), 1);
}

TEST_F(ApplicationTest, TestPushProjectionUnordered) {
    app_.startAcquiring();
    app_.startPreprocessing();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);
    
    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, num_angles_ - 3);
    int overflow = 3;
    pushProjection(num_angles_ - 1, num_angles_ + overflow);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto& projs_ready = app_.rawBuffer().ready();
    EXPECT_EQ(projs_ready[0], 2.f);
    EXPECT_EQ(projs_ready[overflow * pixels_ - 1], 3.f);
    EXPECT_EQ(projs_ready[num_angles_ * pixels_ - 1], 4.f);

    pushProjection(num_angles_ - 3, num_angles_ - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto& projs_front = app_.rawBuffer().front();
    // FIXME: unittest fails from now and then
    EXPECT_THAT(std::vector<float>(projs_front.begin(), projs_front.begin() + 10), 
                Pointwise(FloatNear(1e-6), {0.110098f, -0.272487f, 0.133713f, -0.491590f, 0.520265f,
                                            0.099537f, -0.214807f, 0.464008f, -0.369369f, 0.020631f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()), 
                Pointwise(FloatNear(1e-6), { 0.443812f,  0.056262f, -0.205481f,  0.034181f, -0.328773f,
                                            -0.028346f, -0.080572f, -0.066762f, -0.086848f,  0.262528f}));

    auto& sino = app_.sinoBuffer().ready();
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

TEST_F(ApplicationTest, TestUploading) {
    app_.startPreprocessing();
    app_.startUploading();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ApplicationTest, TestReconstructing) {
    app_.startPreprocessing();
    app_.startUploading();
    app_.startReconstructing();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ApplicationTest, TestWithPagagin) {
    float pixel_size = 1.0f;
    float lambda = 1.23984193e-9f;
    float delta = 1.e-8f;
    float beta = 1.e-10f;
    float distance = 40.f;
    app_.setPaganinParams(pixel_size, lambda, delta, beta, distance);

    app_.startPreprocessing();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);
    
    pushDarks(num_darks_);
    pushFlats(num_flats_);
    // FIXME: fix Paganin
    // pushProjection(0, num_angles_);
}


TEST_F(ApplicationTest, TestDownsampling) {
    app_.startPreprocessing();
    app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, num_angles_);

    app_.onStateChanged(ServerState_State::ServerState_State_READY);
    app_.setDownsamplingParams(2u, 2u);
    // FIXME: std::out_of_range in the dtor of ParallelBeamReconstructor
    // app_.onStateChanged(ServerState_State::ServerState_State_PROCESSING);
    // pushProjection(0, num_angles_);
}

} // namespace recastx::recon::test