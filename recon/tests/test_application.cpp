/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
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
#include "common/queue.hpp"


namespace recastx::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

class MockDaqClient : public DaqClientInterface {

    ThreadSafeQueue<Projection<>> buffer_;

  public:

    MockDaqClient() : DaqClientInterface(1) {}

    void spin() override {}

    void setAcquiring(bool state) override {}

    [[nodiscard]] virtual bool next(Projection<>& proj) override {
        return buffer_.waitAndPop(proj, 100);
    }

    template<typename... Args>
    void push(Args&&... args) {
        buffer_.tryPush(Projection<>(std::forward<Args>(args)...));
    }
};

class MockRampFilter : public Filter {

    int num_cols_;
    int num_rows_;

  public:

    MockRampFilter(int num_cols, int num_rows) : num_cols_(num_cols), num_rows_(num_rows), Filter() {}

    void apply(float* data, int buffer_index) override {
        for (int i = 0; i < num_cols_ * num_rows_; ++i) {
            data[i] += 1.f;
        }
    }
};

class MockRampFilterFactory : public FilterFactory {

    std::unique_ptr<Filter> create(const std::string& name, 
                                   float* data, int num_cols, int num_rows, int buffer_size) override {
        return std::make_unique<MockRampFilter>(num_cols, num_rows);
    }
};

class MockReconstructor: public Reconstructor {

    size_t upload_counter_;
    size_t slice_counter_;
    size_t volume_counter_;

  public:

    MockReconstructor(ProjectionGeometry proj_geom,
                      VolumeGeometry slice_geom, 
                      VolumeGeometry volume_geom,
                      bool double_buffering)
         : upload_counter_(0), slice_counter_(0), volume_counter_(0) {
    }

    void reconstructSlice(Orientation x, int buffer_idx, Tensor<float, 2>& buffer) override { ++slice_counter_; };

    void reconstructVolume(int buffer_idx, Tensor<float, 3>& buffer) override { ++volume_counter_; };

    void uploadSinograms(int buffer_idx, const float* data, size_t n) override { ++upload_counter_; };

    size_t numUploads() const { return upload_counter_; }
    size_t numSlices() const { return slice_counter_; }
    size_t numVolumes() const { return volume_counter_; }
};

class MockReconFactory: public ReconstructorFactory {

  public:

    std::unique_ptr<Reconstructor> create(ProjectionGeometry proj_geom,
                                          VolumeGeometry slice_geom, 
                                          VolumeGeometry volume_geom,
                                          bool double_buffering) {
    return std::make_unique<MockReconstructor>(proj_geom, slice_geom, volume_geom, double_buffering);
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

    size_t buffer_size_ = 100;
    size_t num_angles_ = 16;
    AngleRange angle_range_ = AngleRange::HALF;
    size_t slice_size_ = num_cols_;
    size_t volume_size_ = slice_size_ / 2;

    std::string filter_name_ = "shepp";
    uint32_t threads_ = 2;

    std::unique_ptr<DaqClientInterface> daq_client_;
    std::unique_ptr<FilterFactory> ramp_filter_factory_;
    std::unique_ptr<ReconstructorFactory> recon_factory_;

    size_t num_darks_ = 4;
    size_t num_flats_ = 6;

    const RpcServerConfig rpc_cfg {12347};
    const ImageprocParams imgproc_params {
        threads_, downsampling_col_, downsampling_row_, 0, true, {filter_name_}
    };

    Application app_;

    ApplicationTest() : 
            daq_client_(new MockDaqClient()),
            ramp_filter_factory_(new MockRampFilterFactory()),
            recon_factory_(new MockReconFactory()),
            app_ {buffer_size_, imgproc_params, daq_client_.get(), ramp_filter_factory_.get(), recon_factory_.get(), rpc_cfg} {
        app_.setScanMode(rpc::ScanMode_Mode_DYNAMIC, num_angles_);
    }

    ~ApplicationTest() override {
//        app_.onStateChanged(rpc::ServerState_State_READY);
    };

    void SetUp() override { 
        app_.setProjectionGeometry(recastx::BeamShape::PARALELL, num_cols_, num_rows_,
                                   pixel_width_, pixel_height_, 
                                   src2origin, origin2det, num_angles_, angle_range_);
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
                reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype));
        }
    }

    void pushFlats(int n) {
        std::vector<RawDtype> img(pixels_, 1);
        for (int i = 0; i < n; ++i) {
            dynamic_cast<MockDaqClient*>(daq_client_.get())->push(
                ProjectionType::FLAT, i, num_cols_, num_rows_, 
                reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype));
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
                reinterpret_cast<void*>(img.data()), img.size() * sizeof(RawDtype));
        }
    }
};

TEST_F(ApplicationTest, TestPushProjection) {
    app_.startConsuming();
    app_.startPreprocessing();
    app_.startProcessing();

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
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.60943794f, -0.09861231f, -0.9459102f,  1.f,
                                            -0.38629436f, -0.7917595f,   0.30685282f, -1.1972246f, -0.60943794f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()),
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.38629436f, -1.0794415f, -0.7917595f, -1.1972246f,
                                            -0.9459102f,  -1.1972246f,  -1.1972246f, -1.0794415f, -0.38629436f}));
    EXPECT_THAT(std::vector<float>(sino.begin(), sino.begin() + 10),
                Pointwise(FloatNear(1e-6), {-0.7917595f, -1.0794415f, -1.0794415f, -0.9459102f, -0.09861231f,
                                            -0.9459102f, -1.1972246f, -1.1972246f, -1.0794415f, -0.38629436f}));
    EXPECT_THAT(std::vector<float>(sino.end() - 10, sino.end()),
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.60943794f, -0.09861231f, -0.9459102f, 1.f,
                                            -0.09861231f, -0.7917595f,  -0.38629436f, -1.0794415f, 0.30685282f}));
}

TEST_F(ApplicationTest, TestMemoryBufferReset) {
    app_.startConsuming();
    app_.startPreprocessing();
    app_.startProcessing();

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
    app_.startConsuming();
    app_.startPreprocessing();
    app_.startProcessing();
    
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
    EXPECT_THAT(std::vector<float>(projs_front.begin(), projs_front.begin() + 10), 
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.60943794f, -0.09861231f, -0.9459102f,  1.f,
                                            -0.38629436f, -0.7917595f,   0.30685282f, -1.1972246f, -0.60943794f}));
    EXPECT_THAT(std::vector<float>(projs_front.end() - 10, projs_front.end()), 
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.38629436f, -1.0794415f, -0.7917595f, -1.1972246f,
                                            -0.9459102f,  -1.1972246f,  -1.1972246f, -1.0794415f, -0.38629436f}));

    auto& sino = app_.sinoBuffer().ready();
    EXPECT_THAT(std::vector<float>(sino.begin(), sino.begin() + 10), 
                Pointwise(FloatNear(1e-6), {-0.7917595f, -1.0794415f, -1.0794415f, -0.9459102f, -0.09861231f,
                                            -0.9459102f, -1.1972246f, -1.1972246f, -1.0794415f, -0.38629436f}));
    EXPECT_THAT(std::vector<float>(sino.end() - 10, sino.end()), 
                Pointwise(FloatNear(1e-6), { 0.30685282f, -0.60943794f, -0.09861231f, -0.9459102f, 1.f,
                                            -0.09861231f, -0.7917595f,  -0.38629436f, -1.0794415f, 0.30685282f}));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pushProjection(num_angles_ + overflow, 2 * num_angles_ - 1);
    // trigger warn log message, there must be at least one unfilled group in the buffer
    pushProjection(0, 1);
}

TEST_F(ApplicationTest, TestReconstructing) {
    app_.startConsuming();
    app_.startPreprocessing();
    app_.startUploading();
    app_.startReconstructing();
    app_.startProcessing();

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, num_angles_);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(dynamic_cast<const MockReconstructor*>(app_.reconstructor())->numUploads(), 1);
    EXPECT_EQ(dynamic_cast<const MockReconstructor*>(app_.reconstructor())->numVolumes(), 1);
    EXPECT_EQ(dynamic_cast<const MockReconstructor*>(app_.reconstructor())->numSlices(), 0);
}

// FIXME: fix Paganin
TEST_F(ApplicationTest, TestWithPagagin) {
    // float pixel_size = 1.0f;
    // float lambda = 1.23984193e-9f;
    // float delta = 1.e-8f;
    // float beta = 1.e-10f;
    // float distance = 40.f;
    // app_.setPaganinParams(pixel_size, lambda, delta, beta, distance);

    app_.startConsuming();
    app_.startPreprocessing();
    app_.startProcessing();
    
    // pushDarks(num_darks_);
    // pushFlats(num_flats_);
    // pushProjection(0, num_angles_);
}

TEST_F(ApplicationTest, TestDownsampling) {
    app_.startConsuming();
    app_.startPreprocessing();
    app_.startProcessing();

    pushDarks(num_darks_);
    pushFlats(num_flats_);
    pushProjection(0, num_angles_);

    app_.stopProcessing();
    app_.setDownsampling(2u, 2u);
    app_.startProcessing();
    pushProjection(0, num_angles_);
}

} // namespace recastx::recon::test