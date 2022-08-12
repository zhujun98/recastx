#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <oneapi/tbb.h>

#include "recon/filter.hpp"

namespace tomcat::recon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

class FilterTest : public testing::Test {
  protected:
    int rows_ = 2;
    int cols_ = 5;
    int pixels_ = rows_ * cols_;
    int threads_ = rows_;
    oneapi::tbb::task_arena arena_{threads_};
    std::vector<float> src_ = {1.1, 0.2, 3.5, 2.7, 1.3, 
                               2.5, 0.1, 4.8, 5.2, 0.6,
                               0.4, 0.1, 2.6, 0.7, 1.5, 
                               3.1, 0.2, 3.9, 6.7, 5.6,
                               1.1, 0.2, 3.5, 2.7, 1.3, 
                               2.5, 0.1, 4.8, 5.2, 0.6};
    
};

TEST_F(FilterTest, TestRamlak) {
    EXPECT_THAT(Filter::ramlak(4), 
                Pointwise(FloatNear(1e-6), {0.f, .125f, .25f, .125f}));
    EXPECT_THAT(Filter::ramlak(5), 
                Pointwise(FloatNear(1e-6), {0.f, .08f, .16f, .16f, .08f}));

    auto filter = Filter("ramlak", false, src_.data(), rows_, cols_, threads_);
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, int(src_.size() / pixels_)),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                filter.apply(&src_[i * pixels_], tbb::this_task_arena::current_thread_index());
            }
        });
    });

    EXPECT_THAT(std::vector<float>(src_.begin(), src_.begin() + pixels_),
                Pointwise(FloatNear(1e-6), 
                          {0.02438078f, -0.98966563f, 0.99927864f, 0.25095048f, -0.28494427f,
                           0.74781729f, -1.65816408f, 1.09922602f, 1.28556039f, -1.47443961f}));
    EXPECT_THAT(std::vector<float>(src_.begin() + pixels_, src_.begin() + 2 * pixels_),
                Pointwise(FloatNear(1e-6), 
                          {-0.24394738f, -0.64755418f,  1.02238699f, -0.53799379f, 0.40710835f,
                           -0.05067495f, -1.74595359f,  0.16099689f,  1.12545514f, 0.5101765f}));
    EXPECT_THAT(std::vector<float>(src_.begin() + 2 * pixels_, src_.end()),
                Pointwise(FloatNear(1e-6), 
                          {0.02438078f, -0.98966563f, 0.99927864f, 0.25095048f, -0.28494427f,
                           0.74781729f, -1.65816408f, 1.09922602f, 1.28556039f, -1.47443961f}));
}

TEST_F(FilterTest, TestShepp) {
    EXPECT_THAT(Filter::shepp(4), 
                Pointwise(FloatNear(1e-6), {0.f, 0.11253954f, 0.15915494f, 0.11253954f}));
    EXPECT_THAT(Filter::shepp(5), 
                Pointwise(FloatNear(1e-6), {0.f, 0.07483914f, 0.12109228f, 0.12109228f, 0.07483914f}));

    auto filter = Filter("shepp", false, src_.data(), rows_, cols_, threads_);
    arena_.execute([&]{
        tbb::parallel_for(tbb::blocked_range<int>(0, int(src_.size() / pixels_)),
                          [&](const tbb::blocked_range<int> &block) {
            for (auto i = block.begin(); i != block.end(); ++i) {
                filter.apply(&src_[i * pixels_], tbb::this_task_arena::current_thread_index());
            }
        });
    });
  
    EXPECT_THAT(std::vector<float>(src_.begin(), src_.begin() + pixels_),
                Pointwise(FloatNear(1e-6), 
                          {-0.08023774f, -0.79516008f, 0.82644539f, 0.27944482f, -0.23049239f,
                           0.41235096f, -1.32173338f, 0.944262f, 1.10916587f, -1.14404545f}));
    EXPECT_THAT(std::vector<float>(src_.begin() + pixels_, src_.begin() + 2 * pixels_),
                Pointwise(FloatNear(1e-6), 
                          {-0.23537546f, -0.51160547f, 0.8112198f, -0.36250355f, 0.29826468f,
                           -0.14364247f, -1.53828898f, 0.09308264f, 1.05090197f, 0.53794685f}));
    EXPECT_THAT(std::vector<float>(src_.begin() + 2 * pixels_, src_.end()),
                Pointwise(FloatNear(1e-6), 
                          {-0.08023774f, -0.79516008f, 0.82644539f, 0.27944482f, -0.23049239f,
                           0.41235096f, -1.32173338f, 0.944262f, 1.10916587f, -1.14404545f}));
}

} // tomcat::recon::test