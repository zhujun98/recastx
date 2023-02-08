#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/slice_mediator.hpp"

namespace tomcat::recon::test {

class SliceMediatorTest : public testing::Test {

protected:

    size_t slice_size_;

    SliceMediator mediator_;

    SliceMediatorTest() : slice_size_{6}, mediator_{} {
        mediator_.resize({slice_size_, slice_size_});
    }

};

TEST_F(SliceMediatorTest, TestGeneral) {

}

} // tomcat::recon::test