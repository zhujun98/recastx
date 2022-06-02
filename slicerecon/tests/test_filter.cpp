#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slicerecon/filter.hpp"

namespace slicerecon::test {

using ::testing::Pointwise;
using ::testing::FloatNear;

class FilterTest : public testing::Test {
  protected:
    int rows_ = 20;
    int cols_ = 12;
    int pixels_ = rows_ * cols_;
    std::vector<float> src_;
    std::vector<float> dst_;

    void SetUp() {
        src_.resize(pixels_);
        std::fill(src_.begin(), src_.end(), 1.f);

        dst_.resize(pixels_);
        std::fill(dst_.begin(), dst_.end(), 1.f);
    }    
};

TEST_F(FilterTest, TestShepp) {
    auto filter = Filter("shepp", false, src_.data(), rows_, cols_);
    filter.apply(&dst_[0]);

    EXPECT_THAT(std::vector<float>(dst_.begin(), dst_.begin() + 2), 
                Pointwise(FloatNear(1e-6), {1.0, 1.0}));
}

TEST_F(FilterTest, TestRamlak) {
    auto filter = Filter("ramlak", false, src_.data(), rows_, cols_);
    filter.apply(&dst_[0]);

    EXPECT_THAT(std::vector<float>(dst_.begin(), dst_.begin() + 2),
                Pointwise(FloatNear(1e-6), {1.0, 1.0}));
}

}